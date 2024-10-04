# Tag Name: `cfpdf`

## Description
Manipulates existing PDF documents. The following list describes some of the
 tasks you can perform with the cfpdf tag:
 Merge several PDF documents into one PDF document.
 Extract pages from multiple PDF documents and generate a new PDF document.
 Linearize multipage PDF documents for faster display.
 Encrypt or decrypt PDF files for security.

## Syntax
```cfml
<cfpdf>
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The action to take.

### Attribute: `ascending`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `no`
- **Description**: Order in which the PDF files are sorted:
 yes: files are sorted in ascending order
 no: files are sorted in descending order

### Attribute: `copyfrom`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The filename of the PDF document from which to copy the watermark

### Attribute: `destination`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The pathname of the modified PDF document.
 If the destination file exists, you must set the overwrite attribute to yes.
 If the destination file does not exist, ColdFusion creates it as long as
 the parent directory exists. (optional)

### Attribute: `directory`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the directory of the PDF documents to merge.
 You must specify either the directory or the source.
 If you specify the directory, you must also specify the order.

### Attribute: `encrypt`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the type of encryption used on the source PDF document

### Attribute: `flatten`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `no`
- **Description**: Specify whether the output file is flattened:
 yes: the formatting is removed and the file is flattened
 no: the format of the source PDF is maintained in the output file.

### Attribute: `foreground`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `no`
- **Description**: Specify whether the watermark is placed in the foreground of the PDF document:
 yes: the watermark appears in the foreground
 no: the watermark appears in the background

### Attribute: `image`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the image used as a watermark.
 You can specify a filename or a ColdFusion image variable.

### Attribute: `info`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specify the structure variable for relevant information, for example, #infoStruct#.
 ColdFusion ignores read only information, such as the creation date, application used to create
 the PDF document, and encryption parameters.

### Attribute: `isBase64`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `no`
- **Description**: Specify whether the image used a watermark is in Base64 format:
 yes: the image is in Base64 format
 no: the image is not in Base64 format

### Attribute: `keepbookmark`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specify whether bookmarks from the source PDF
 documents are retained in the merged document:
 yes: the bookmarks are retained
 no: the bookmarks are removed

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the PDF document variable name, for example, myPDFdoc.
 If the source is a PDF document variable, you cannot specify the
 name attribute again; you can write the modified PDF document
 to the destination.

### Attribute: `newOwnerPassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the password for the owner of the PDF document.

### Attribute: `newUserPassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the password for the user of the PDF document.

### Attribute: `opacity`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `3`
- **Description**: Specify the opacity of the watermark.
 Valid values are integers in the range 0 (transparent)
 through 10 (opaque).

### Attribute: `order`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specify the order in which the PDF documents
 in the directory are merged:
 name: orders the documents alphabetically
 time: orders the documents by timestamp

### Attribute: `overwrite`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify whether to overwrite the destination file:
 yes: overwrites the destination file
 no: does not overwrite the destination file

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the owner or user password of the source PDF document, if it exists.

### Attribute: `permissions`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the type of permissions on the PDF document:
 AllowPrintHigh
 AllowPrintLow
 AllowModify
 AllowCopy
 AllowAdd
 AllowSecure
 AllowModifyAnnotations
 AllowExtract
 AllowFillIn
 all
 none
 Except for all or none, you can specify a
 comma separated list of permissions. (optional)

### Attribute: `position`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the position on the page where
 the watermark is placed. The position represents the
 top-left corner of the watermark.
 Specify the x and y coordinates; for example 50,30.

### Attribute: `rotation`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the degree of rotation of
 the watermark image on the page; for example, 30.

### Attribute: `showonprint`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify whether the watermark is printed with
 the PDF document:
 yes: the watermark is printed with the PDF document
 no: the watermark is not printed with the PDF document

### Attribute: `source`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specify the source. The source can be:
 The pathname to a PDF document; for example, c:\work\myPDF.pdf
 A PDF document variable in memory that is generated by the
 cfdocument tag or the cfpdf tag; for example, #myPDFdoc#
 The binary content of PDF document variable.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the type to remove from the source PDF document:
 attachment
 bookmark
 watermark

### Attribute: `version`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (write) Specify the version of the PDF document to write.

### Attribute: `transparent`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specifies whether the image background is transparent or opaque: format=png only
 * yes: the background is transparent.
 * no: the background is opaque.

### Attribute: `resolution`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Image quality used to generate thumbnail images:
 * high: use high resolution (uses more memory).
 * low: use low resolution.

### Attribute: `stoponerror`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Valid only if the directory attribute is specified. If the specified directory contains files other then ColdFusion-readable PDF files, ColdFusion either stops merge process or continues.
 * yes: stops the merge process if invalid PDF files exist in the specified directory.
 * no: continues the merge process even if invalid files exist in the specified directory.

### Attribute: `inputfiles`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Structure that maps the PDF source files to the input variables in the DDX file, or a string of elements and their pathname.

### Attribute: `scale`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Size of the thumbnail relative to the source page. The value represents a percentage from 1 through 100.

### Attribute: `imageprefix`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Prefix used for each image thumbnail file generated. The image filenames use the format: imagePrefix_page_n.format.

### Attribute: `outputfiles`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Structure that contains the output files in the DDX file or string as keys and the pathname to the result file as the value.

### Attribute: `pages`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Page or pages in the source PDF document on which to perform the action. You can specify multiple pages and page ranges as follows: "1,6-9,56-89,100, 110-120".

### Attribute: `ddxfile`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Owner or user password of the source PDF document, if the document is password-protected.

### Attribute: `saveoption`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `full`
- **Description**: Save options for the PDF output:
 * full: normal save (default)
 * incremental: required to save modifications to a signed PDF document.
 * linear: for faster display.

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `jpg`
- **Description**: File type of thumbnail image output

### Attribute: `hires`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Sets a high resolution for the thumbnail if set to yes.

### Attribute: `maxScale`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the maximum scale of the thumbnail

### Attribute: `maxBreadth`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF9+ Specifies maximum width of the thumbnail

### Attribute: `maxLength`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the maximum length of the thumbnail

### Attribute: `compressTIFFs`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Compress thumbnail which are in TIFF format.

### Attribute: `overridepage`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify whether to override page or not

### Attribute: `package`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Create PDF packages

### Attribute: `hScale`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Horizontal scale of the image to be modified. Valid values are hscale<1.

### Attribute: `vScale`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Vertical scale of the image to be modified. Valid values are vscale>0.

### Attribute: `noBookMarks`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF9+ Remove bookmarks from PDF document

### Attribute: `noAttachments`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF9+ Removes all attachments from PDF documents.

### Attribute: `noComments`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Remove comments from PDF document

### Attribute: `noJavaScripts`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF9+ Remove all document level JavaScript actions

### Attribute: `noLinks`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Remove external cross-references

### Attribute: `noMetadata`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF9+ Remove document information and metadata

### Attribute: `noThumbnails`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Remove embedded page thumbnails

### Attribute: `noFonts`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF9+ Remove font styling

### Attribute: `algo`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF9+ Specifies the algorithm for image downsampling.

### Attribute: `topMargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the value of the top margin

### Attribute: `leftMargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the value of the left margin

### Attribute: `rightMargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the value of the right margin

### Attribute: `bottomMargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the value of the bottom margin

### Attribute: `numberFormat`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the numbering format for PDF pages in the footer.

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Aligns the header and footer in PDF.

### Attribute: `honourspaces`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Set this option to "true" if you need characters to be converted to spaces.

### Attribute: `addQuads`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Add the position or quadrants of the thumbnail

### Attribute: `text`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify Text Value

### Attribute: `useStructure`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF9+ Specify whether to use structure or not

### Attribute: `jpgdpi`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF9+

### Attribute: `encodeAll`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Encode streams that are not encoded to optimize page content

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

