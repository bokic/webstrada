/**
 * @file cfimage.h
 * @brief Image payload and image built-in function declarations.
 *
 * Provides the ImageData payload stored in a cfvariant of type Image and the
 * CFML image built-in functions implemented on a cairo backend:
 * ImageNew, ImageRead, ImageReadBase64, ImageWrite, ImageWriteBase64,
 * ImageGetBlob, ImageGetWidth, ImageGetHeight, ImageInfo, ImageGetMetadata,
 * GetReadableImageFormats, GetWriteableImageFormats, IsImageFile.
 */

#pragma once

#include "cfvariant.h"

#include <cairo.h>

#include <cstdint>
#include <string>
#include <vector>


namespace webstrada
{

// Payload of a cfvariant of type Image. Refcounted because image variables
// share their pixel buffer on assignment (CF's `img2 = img` keeps the same
// image; mutating one is visible through the other). The pixel storage is a
// cairo image surface in premultiplied ARGB32. colormodel is one of "rgb",
// "argb", "grayscale" and records the CF color model the image was created or
// read as; colormodelType ("PackedColorModel"/"ComponentColorModel") is what
// ImageInfo reports. source is the path the image was read from ("" for blank
// images) and is where ImageGetMetadata looks.
struct ImageData
{
    int refcount = 1;
    cairo_surface_t *surface = nullptr;
    int width = 0;
    int height = 0;
    std::string colormodel;       // "rgb" | "argb" | "grayscale"
    std::string colormodelType;   // "PackedColorModel" | "ComponentColorModel"
    std::string source;           // source path ("" when blank)
    std::string sourceFormat;     // "png" | "jpeg" | "gif" | "bmp" | "pnm" | "" when blank
    // Raw bytes the image was decoded from (used by ImageGetMetadata). Empty
    // for blank images.
    std::vector<std::byte> sourceBytes;

    // ------------------------------------------------------------------
    // Drawing state (used by the ImageDraw* / ImageSet* functions).
    // Defaults were verified against ColdFusion 2025:
    //  - fresh ImageNew("",w,h,"rgb") canvas is black,
    //  - default drawing color is white,
    //  - default background color is black,
    //  - antialiasing defaults to off,
    //  - transparency is a percentage 0..100 applied with SrcOver alpha
    //    (alpha = 1 - percent/100),
    //  - XOR mode replaces each covered pixel with
    //    (drawingColor ^ xorColor ^ destination) so drawing twice erases.
    // ------------------------------------------------------------------
    uint32_t drawingColor = 0xFFFFFFFFu;   // 0xRRGGBB
    uint32_t backgroundColor = 0xFF000000u;// 0xRRGGBB
    int transparency = 0;                  // 0..100 percent
    bool antialias = false;                // off by default
    bool xorMode = false;
    uint32_t xorColor = 0xFF000000u;       // 0xRRGGBB
    double strokeWidth = 1.0;
    std::string strokeCaps = "butt";       // "butt" | "round" | "square"
    std::string strokeJoins = "miter";     // "miter" | "round" | "bevel"
    double strokeMiterLimit = 10.0;
    std::vector<double> strokeDash;        // empty = solid line
    double strokeDashPhase = 0.0;

    // Drawing-axis transform accumulated by ImageTranslateDrawingAxis /
    // ImageRotateDrawingAxis / ImageShearDrawingAxis. Mirrors the Java2D
    // Graphics2D transform: user-space drawing coordinates are mapped to
    // image space by this matrix, and each axis call post-multiplies
    // (M = M * T) so the newest transform applies to the drawing coordinates
    // first. Positive rotation angles are clockwise on screen. A fresh image
    // starts with the identity.
    cairo_matrix_t drawingTransform{1, 0, 0, 1, 0, 0};
    bool hasDrawingTransform = false;
};

ImageData *image_data_retain(ImageData *img);
void image_data_release(ImageData *img);

// Returns the ImageData of an Image variant, throwing when the value is not
// an image (CF: java.lang.ClassCastException). Caller may pass nullptr for
// null-pointer guard; a missing image also throws.
ImageData *image_from_variant(const cfvariant *v);

}

namespace cfml
{

using namespace webstrada;

cfvariant *cf_imagenew(const cfvariant *source, const cfvariant *width, const cfvariant *height,
                       const cfvariant *imageType, const cfvariant *canvasColor);
cfvariant *cf_imageread(const cfvariant *path);
cfvariant *cf_imagereadbase64(const cfvariant *data);
cfvariant *cf_imagewrite(const cfvariant *image, const cfvariant *destination, const cfvariant *quality, const cfvariant *overwrite);
cfvariant *cf_imagewritebase64(const cfvariant *image, const cfvariant *destination, const cfvariant *format,
                               const cfvariant *inHTMLFormat, const cfvariant *overwrite);
cfvariant *cf_imagegetblob(const cfvariant *image);
cfvariant *cf_imagegetwidth(const cfvariant *image);
cfvariant *cf_imagegetheight(const cfvariant *image);
cfvariant *cf_imageinfo(const cfvariant *image);
cfvariant *cf_imageclone(const cfvariant *image);
cfvariant *cf_imagegetmetadata(const cfvariant *image);
cfvariant *cf_imagecreatecaptcha(const cfvariant *height, const cfvariant *width, const cfvariant *text,
                                 const cfvariant *difficulty, const cfvariant *font, const cfvariant *fontsize);
cfvariant *cf_imagegetbufferedimage(const cfvariant *name);
cfvariant *cf_imagegetexifmetadata(const cfvariant *name);
cfvariant *cf_imagegetexiftag(const cfvariant *name, const cfvariant *tagname);
cfvariant *cf_imagegetiptcmetadata(const cfvariant *name);
cfvariant *cf_imagegetiptctag(const cfvariant *name, const cfvariant *tagname);
cfvariant *cf_getreadableimageformats();
cfvariant *cf_getwriteableimageformats();
cfvariant *cf_isimagefile(const cfvariant *value, const cfvariant *format);

// <cfimage> tag runtime helper: `attrs` is a struct of the evaluated tag
// attributes, `variables` the variables scope (for name/structname) and `out`
// the output buffer (for writetobrowser / inline captcha).
cfvariant *cf_cfimage(const cfvariant *attrs, void *variables, void *out);

// Drawing primitives and drawing-state setters.
cfvariant *cf_imageclearrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                             const cfvariant *width, const cfvariant *height);
cfvariant *cf_imagedrawarc(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                           const cfvariant *width, const cfvariant *height, const cfvariant *startAngle,
                           const cfvariant *arcAngle, const cfvariant *filled);
cfvariant *cf_imagedrawbeveledrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                                   const cfvariant *width, const cfvariant *height, const cfvariant *raised,
                                   const cfvariant *filled);
cfvariant *cf_imagedrawcubiccurve(const cfvariant *image, const cfvariant *ctrlx1, const cfvariant *ctrly1,
                                  const cfvariant *ctrlx2, const cfvariant *ctrly2, const cfvariant *x1,
                                  const cfvariant *y1, const cfvariant *x2, const cfvariant *y2);
cfvariant *cf_imagedrawline(const cfvariant *image, const cfvariant *x1, const cfvariant *y1,
                            const cfvariant *x2, const cfvariant *y2);
cfvariant *cf_imagedrawlines(const cfvariant *image, const cfvariant *xcords, const cfvariant *ycords,
                             const cfvariant *isPolygon, const cfvariant *filled);
cfvariant *cf_imagedrawoval(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                            const cfvariant *width, const cfvariant *height, const cfvariant *filled);
cfvariant *cf_imagedrawpoint(const cfvariant *image, const cfvariant *x, const cfvariant *y);
cfvariant *cf_imagedrawquadraticcurve(const cfvariant *image, const cfvariant *x1, const cfvariant *y1,
                                      const cfvariant *ctrlx1, const cfvariant *ctrly1, const cfvariant *x2,
                                      const cfvariant *y2);
cfvariant *cf_imagedrawrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                            const cfvariant *width, const cfvariant *height, const cfvariant *filled);
cfvariant *cf_imagedrawroundrect(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                                 const cfvariant *width, const cfvariant *height, const cfvariant *arcwidth,
                                 const cfvariant *archeight, const cfvariant *filled);
cfvariant *cf_imagedrawtext(const cfvariant *image, const cfvariant *str, const cfvariant *x,
                            const cfvariant *y, const cfvariant *attributes);
cfvariant *cf_imagerotatedrawingaxis(const cfvariant *image, const cfvariant *angle, const cfvariant *x,
                                     const cfvariant *y);
cfvariant *cf_imagesheardrawingaxis(const cfvariant *image, const cfvariant *shrx, const cfvariant *shry);
cfvariant *cf_imagetranslatedrawingaxis(const cfvariant *image, const cfvariant *x, const cfvariant *y);
cfvariant *cf_imagesetantialiasing(const cfvariant *image, const cfvariant *antialias);
cfvariant *cf_imagesetbackgroundcolor(const cfvariant *image, const cfvariant *color);
cfvariant *cf_imagesetdrawingcolor(const cfvariant *image, const cfvariant *color);
cfvariant *cf_imagesetdrawingstroke(const cfvariant *image, const cfvariant *attributes);
cfvariant *cf_imagesetdrawingtransparency(const cfvariant *image, const cfvariant *percent);
cfvariant *cf_imagexordrawingmode(const cfvariant *image, const cfvariant *color);

// Pixel / geometry operations (Group A: ImageAddBorder, ImageBlur, ImageCopy,
// ImageCrop, ImageFlip, ImageGrayscale, ImageMakeColorTransparent,
// ImageMakeTranslucent, ImageNegative, ImageOverlay, ImagePaste, ImageResize,
// ImageRotate, ImageScaleToFit, ImageSharpen, ImageShear, ImageTranslate).
cfvariant *cf_imageaddborder(const cfvariant *image, const cfvariant *thickness,
                             const cfvariant *color, const cfvariant *borderType);
cfvariant *cf_imageblur(const cfvariant *image, const cfvariant *blurRadius);
cfvariant *cf_imagecopy(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                        const cfvariant *width, const cfvariant *height,
                        const cfvariant *dx, const cfvariant *dy);
cfvariant *cf_imagecrop(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                        const cfvariant *width, const cfvariant *height);
cfvariant *cf_imageflip(const cfvariant *image, const cfvariant *transpose);
cfvariant *cf_imagegrayscale(const cfvariant *image);
cfvariant *cf_imagemakecolortransparent(const cfvariant *image, const cfvariant *color);
cfvariant *cf_imagemaketranslucent(const cfvariant *image, const cfvariant *percent);
cfvariant *cf_imagenegative(const cfvariant *image);
cfvariant *cf_imageoverlay(const cfvariant *image1, const cfvariant *image2,
                           const cfvariant *rule, const cfvariant *alpha);
cfvariant *cf_imagepaste(const cfvariant *image1, const cfvariant *image2,
                         const cfvariant *x, const cfvariant *y);
cfvariant *cf_imageresize(const cfvariant *image, const cfvariant *width, const cfvariant *height,
                          const cfvariant *interpolation, const cfvariant *blurFactor);
cfvariant *cf_imagerotate(const cfvariant *image, const cfvariant *x, const cfvariant *y,
                          const cfvariant *angle, const cfvariant *interpolation);
cfvariant *cf_imagescaletofit(const cfvariant *image, const cfvariant *fitWidth, const cfvariant *fitHeight,
                              const cfvariant *interpolation, const cfvariant *blurFactor);
cfvariant *cf_imagesharpen(const cfvariant *image, const cfvariant *gain);
cfvariant *cf_imageshear(const cfvariant *image, const cfvariant *shear,
                         const cfvariant *direction, const cfvariant *interpolation);
cfvariant *cf_imagetranslate(const cfvariant *image, const cfvariant *xTrans, const cfvariant *yTrans,
                             const cfvariant *interpolation);

}
