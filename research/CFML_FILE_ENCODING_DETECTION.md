# Research: How ColdFusion detects page encoding (`cfprocessingdirective pageEncoding`)

## Summary

ColdFusion resolves a CFML page's input encoding at **compile time**, in three
phases, before and during parsing:

1. **BOM detection** on the raw bytes wins over everything. If a BOM is present
   the page *must* be read as that encoding.
2. **No BOM -> ICU charset auto-detection** with a strict confidence threshold;
   on failure it falls back to the Java platform default (`file.encoding`).
3. **`cfprocessingdirective pageEncoding="…"`** (a compile-time, string-literal
   only directive) *overrides* the chosen encoding by **re-reading the raw page
   bytes from offset 0 and re-parsing the entire page** — unless a BOM
   contradicts it, which is a compile error.

The `pageencoding` attribute never reaches runtime code: it is verified and then
deleted from the AST during parsing. The only runtime artifact is that the final
page encoding is stored on the `PageContext` (and inherited by included pages).

This document is distilled from the decompiled ColdFusion 2025 sources in
`tmp/cfdecomp/src/cfusion/sources/`.

## 1. Initial read — `TemplateReader` / `BOMReader`

File: `coldfusion/compiler/TemplateReader.java` (extends
`coldfusion/util/BOMReader.java`).

`NeoTranslationContext.getPageReader()` returns a `TemplateReader` over the raw
file bytes (`NeoTranslationContext.java:529`). The constructor classifies the
bytes in this order:

1. **Compiled class** — first 4 bytes `CAFEBABE` -> it's a `.class` file, not a
   CF template (`isClasses = true`).
2. **Legacy CF template headers** (`TemplateReader.java:20-24`) — throws
   `UnsupportedEncodingException`:
   - `Allaire Cold Fusion Template\nHeader Size: `
   - `Allaire Cold Fusion Template\nHeader Size: New Version`
   - `Adobe Cold Fusion Template\nHeader Size: New Version2`
   - `ColdFusion Report Template` (this one is instead *decoded* via
     `ReportDecoder.getReport()` and then treated as the page body; these are
     encrypted report templates).
3. **BOM detection** (`BOMReader.getBOMEncoding`, `BOMReader.java:86`). Only
   three BOMs are honored, and each sets `explicitEncodingSet = true`:

   | BOM bytes       | Encoding (canonical Java name) | Notes |
   |-----------------|--------------------------------|-------|
   | `EF BB BF`      | `UTF8`                         | UTF-8 |
   | `FF FE`         | `UnicodeLittleUnmarked` (UTF-16LE) | UTF-16 little-endian |
   | `FE FF`         | `UnicodeBigUnmarked` (UTF-16BE) | UTF-16 big-endian |

   No UTF-32, UTF-7, EBCDIC, SCSU, BOCU-1, or GB18030 BOMs are recognized by
   the page reader. The same three constants are reused by `<cffile>` reads
   (`FileUtils.java:1066`), `cfloop file` (`LoopTag.java:132`), `cfinclude`
   (`IncludeTag.java:504`) and `<cfhttp>` response decoding
   (`HttpTag.java:1517`). Quirk: because `getBOMEncoding` checks `FF FE` and
   `FE FF` directly, a UTF-32LE file (`FF FE 00 00`) is misdetected as UTF-16LE.

   **BOM variants ColdFusion does NOT support.** The decompiled sources contain
   exactly these three BOM constants (`BOMReader.java:14-16`) and no others;
   there is no code anywhere in the reader that recognizes any other BOM. The
   standard BOM signatures (Unicode/WHATWG table) that fall outside that set —
   and therefore must be detected by us and rejected — are:

   | BOM bytes       | Variant     | Notes on ColdFusion behavior |
   |-----------------|-------------|------------------------------|
   | `00 00 FE FF`   | UTF-32 BE   | no CF signature matches -> falls through to ICU detection |
   | `FF FE 00 00`   | UTF-32 LE   | first 2 bytes `FF FE` match -> **misdetected as UTF-16LE**, remaining `00 00` becomes part of the decoded text |
   | `2B 2F 76 38`   | UTF-7 (1)   | falls through to ICU detection |
   | `2B 2F 76 39`   | UTF-7 (2)   | falls through to ICU detection |
   | `2B 2F 76 2B`   | UTF-7 (3)   | falls through to ICU detection |
   | `2B 2F 76 2F`   | UTF-7 (4)   | falls through to ICU detection |
   | `2B 2F 76 38 2D`| UTF-7 (5)   | falls through to ICU detection |
   | `F7 64 4C`      | UTF-1       | falls through to ICU detection |
   | `DD 73 66 73`   | UTF-EBCDIC  | falls through to ICU detection |
   | `0E FE FF`      | SCSU        | falls through to ICU detection |
   | `FB EE 28`      | BOCU-1      | falls through to ICU detection |
   | `84 31 95 33`   | GB-18030    | falls through to ICU detection |

   Important: ColdFusion itself does **not** raise an error for any of these —
   unrecognized BOM bytes simply flow on to the ICU charset detector (step 4)
   and are usually mis-decoded. The only byte-prefix checks in `TemplateReader`
   that *do* throw `UnsupportedEncodingException` are the legacy encoded-CFML
   template headers in step 2 (`Allaire Cold Fusion Template\nHeader Size: …`,
   `Adobe Cold Fusion Template\nHeader Size: New Version2`). If we want to
   reject the unsupported BOMs above deterministically (instead of relying on
   ICU detection + confidence heuristics), we must add our own signature check
   for them and throw an exception before tokenizing.

4. **No BOM -> charset auto-detection** (`EncodingDetector`,
   `coldfusion/util/EncodingDetector.java`).

   ### What "ICU charset auto-detection" is

   It is ColdFusion's wrapper (`coldfusion.util.EncodingDetector`) around the
   **ICU4J** library's `com.ibm.icu.text.CharsetDetector` / `CharsetMatch`
   classes. ICU4J is a third-party (bundled, non-decompiled) library, so the
   detector internals below are described from the ICU algorithm; everything
   else is from the decompiled `EncodingDetector.java`.

   The detector is a *statistical* charset guesser (the same technique as
   Mozilla's "universalchardet"): it feeds the raw bytes through several
   charset-detection heuristics and scores each candidate on a 0-100
   confidence scale:

   - **Multi-byte charsets** (UTF-8, UTF-16/UTF-32, Shift_JIS, EUC-JP, EUC-KR,
     Big5, GB18030, …) are recognized by byte-distribution analysis — how
     closely the observed lead/trail byte frequencies and sequence validity
     match each charset's rules (e.g. valid UTF-8 sequences and code-point
     ranges).
   - **Single-byte charsets** (windows-1252, ISO-8859-x, …) are scored with
     n-gram language models that compare byte frequencies against per-language
     tables (the detector returns a name like `windows-1252`, not the abstract
     ISO name).
   - **Escape-sequence charsets** (ISO-2022-JP/-KR/-CN, HZ-GB-2312) are found
     by scanning for their mandatory escape sequences.
   - `setDeclaredEncoding(name)` tells ICU to prefer a charset already declared
     elsewhere (not used by `TemplateReader`; the field stays `null`).
   - `enableInputFilter(true)` makes ICU strip HTML/XML tags and comments from
     the sample before scoring, so markup does not skew the text statistics —
     CF turns this on for page reading (`new EncodingDetector(true)`).

   ### How ColdFusion wires it (`EncodingDetector.java`)

   - `EncodingDetector` constructor stores the confidence threshold
     (`MIN_CONFIDENCE_THRESHHOLD`), the input-filter flag, and an optional
     `declaredEncoding`. The threshold defaults to
     `DEFAULT_MIN_CONFIDENCE_THRESHHOLD = 100` and is overridable per JVM via
     the system property `coldfusion.charsetdetection.minthreshhold` (a value
     outside 1..100 silently forces 100 back).
   - `detectEncoding(BufferedInputStream stream)` builds a fresh
     `CharsetDetector`, calls `detector.setText(stream)` (when the system
     property `coldfusion.compile.encoding.detect.readfully=true` is set, the
     whole file is first slurped into a `byte[]` with `IOUtils.toByteArray(in)`
     and `detectEncoding(byte[])` is used instead — default off, detection
     reads directly from the buffered stream).
   - `detector.detect()` returns the single best `CharsetMatch`. ColdFusion
     accepts it **only if both**:
     - `match.getConfidence() >= threshold` (default 100 → a *certain* match),
       and
     - `Charset.isSupported(match.getName())` (the JVM must actually have the
       charset).
     Otherwise `detectEncoding` returns `null` and the caller falls back.
   - Fallback chain (`getReader(stream, defaultEncoding, providedEncoding)`):
     1. a caller-supplied `providedEncoding` (e.g. the `charset` attribute of
        `cffile`) wins outright — `new InputStreamReader(stream, providedEncoding)`;
     2. else detect; if non-null → `new InputStreamReader(stream, detected)`;
     3. else a caller `defaultEncoding` if non-blank;
     4. else `new InputStreamReader(stream)` → the **JVM platform default**
        (`file.encoding`, typically UTF-8).
   - `getDetectableCharsets(true)` exposes the candidate list:
     `CharsetDetector.getAllDetectableCharsets()` filtered by
     `Charset.isSupported`.

   ### Why this matters for CFML pages

   - Pure-ASCII content is *always* a 100%-confidence UTF-8 match, so ordinary
     pages are read as UTF-8 without a BOM.
   - Latin-1 pages (bytes 0x80-0xFF) usually score best as windows-1252 /
     ISO-8859-1 but typically **below 100** confidence, so detection returns
     `null` and the page falls back to the platform default (UTF-8) — which
     mis-decodes the high bytes. This is exactly why Latin-1 pages need an
     explicit `<cfprocessingdirective pageEncoding="iso-8859-1">` (or a BOM).
   - UTF-16/32 without a BOM is effectively not detectable and falls back the
     same way.
   - If no detection succeeds, `-Dfile.usesystemencoding=true` forces the
     platform default instead of even trying detection.
   - The same `EncodingDetector` is reused for `<cffile action="read">` /
     properties files (`FileUtils.java:457`) and the internal INI reader
     (`IniUtils.java:155`); both pass an explicit charset when one is given,
     which takes priority over detection.

`TemplateReader.resetEncoding(desiredEncoding, tc)` (`TemplateReader.java:139`)
can rewind the stream (re-opening the file if the mark was invalidated) and
rebuild the reader as `new InputStreamReader(in, desiredEncoding)`.

The resolved encoding flows into parsing in `NeoTranslator.parsePage`
(`NeoTranslator.java:410-424`):

```java
ASCII_CharStream charStream = new ASCII_CharStream(in);
cfml40 parser = new cfml40(charStream);
parser.setInputEncoding(in.getEncoding());
tc.setPageEncoding(in.getEncoding());
```

## 2. `cfprocessingdirective pageEncoding` — compile-time only

Two grammar productions in `coldfusion/compiler/cfml40.java`:

- the tag form `<cfprocessingdirective pageEncoding="…">` ->
  `cfprocessingdirective_startTag()` (line 7393);
- the component/script form `[cfprocessingdirective pageEncoding="…"]` ->
  `processingdirective()` (line 7307, consumes the raw `PAGEENCODING` token).

Both do the same thing:

1. Fetch the `PAGEENCODING` attribute expression.
2. Call `verifyPageEncoding(encoding)` (`CFMLParserBase.java:1818`):

   ```java
   String desiredEncoding = EvaluateEngine._String(encoding);   // constant-fold
   if (inputEncoding == null || !BOMReader.isEncodingMatch(desiredEncoding, inputEncoding)) {
       throw new ConflictingEncodingSpecificationException(desiredEncoding, inputEncoding);
   }
   new InputStreamReader(new StringBufferInputStream(""), desiredEncoding).getEncoding();
   // UnsupportedEncodingException -> InvalidEncodingSpecificationException
   ```

   - `EvaluateEngine._String` constant-folds the expression at **compile time**,
     so `pageEncoding` must be a string literal (a variable reference throws
     `NonConstantExpressionException`). This matches the docs: "A string literal;
     cannot be a variable."
   - `BOMReader.isEncodingMatch(enc1, enc2)` canonicalizes `enc1` via
     `new InputStreamReader(new StringBufferInputStream(""), enc1).getEncoding()`
     and compares to `enc2`; additionally `utf16`/`utf-16` is accepted as
     matching either `UnicodeLittleUnmarked` or `UnicodeBigUnmarked`.
   - The final `getEncoding()` call validates the name; an unknown charset ->
     `InvalidEncodingSpecificationException`, message:
     `The specified page encoding, {desiredEncoding}, is not supported.`

3. **On success (match), the `pageencoding` attribute is deleted from the AST**
   (`jjtn000.removeAttrNode("PAGEENCODING")`). If no other attributes remain
   the node is popped entirely — the emitted `cfprocessingdirective` tag then
   only ever carries `suppressWhitespace` logic (`ProcessingDirectiveTag`).

## 3. Re-parse on conflict — `NeoTranslator.parsePage`

`NeoTranslator.java:470-478`:

```java
} catch (ConflictingEncodingSpecificationException cex2) {
    if (!in.explicitEncodingSet()) {
        in.resetEncoding(cex2.desiredEncoding, tc);   // rewind + new InputStreamReader
        tc.setPageEncoding(cex2.desiredEncoding);
        tc.resetPropertyTable();
        result = parsePage(tc, in);                   // re-parse the whole page
    } else {
        throw cex2;
    }
}
```

- **No BOM** -> the page is re-read from byte 0 with `desiredEncoding` and the
  **entire page is re-parsed** (the directive may appear anywhere, but the whole
  file is re-read so all earlier text is decoded with the corrected charset).
- **BOM present** (`explicitEncodingSet() == true`) -> the exception is
  rethrown -> compile error, message:
  `Cannot use the charset {desiredEncoding} because the file has a Byte Order Mark indicating it uses the charset {inputEncoding}.`

The same two-phase path exists for component templates using a separate
`pageReader` (`NeoTranslator.java:427-461`).

### No "first 4096 bytes" search window

There is **no** logic that searches for `cfprocessingdirective` only within the
first 4096 bytes of the file. `cfprocessingdirective` appears in the decompiled
sources only in the grammar (`cfml40.java`) and the token table
(`cfml40Constants.java`); there is no separate head-of-file scan for it. The
conflict exception fires wherever the tag is tokenized and the whole page is
re-read from byte 0, so the directive may legally appear anywhere in the page.
The 4096 that shows up in the compiler is only the lexer buffer size
(`ASCII_CharStream`, `ASCII_CharStream.java:218`) and is unrelated to the
directive. The only head-of-file byte scanning in the whole pipeline is the
BOM/legacy-header signature check in `TemplateReader`/`BOMReader.startsWith`
(max a few dozen bytes). (The commonly-cited "4096" claim likely conflates this
with ICU's `CharsetDetector.setText(InputStream)` reading a bounded prefix for
*encoding detection*, or with Lucee's implementation.)

## 4. Runtime side effects

- `TemplateAssembler.assemblePage` (`TemplateAssembler.java:291`) emits
  `pageContext.setPageEncoding(tc.getPageEncoding())` at page entry; the value is
  stored in `NeoPageContext.pageEncoding` and propagated to child page contexts
  on include (`NeoPageContext.java:178`). In the decompiled sources it is only
  stored/read — it does not itself drive the response output.
- The **response** charset is a separate, admin-panel-configured setting:
  `defaultCharset` in `RuntimeServiceImpl` (`RuntimeServiceImpl.java:244`,
  default `"UTF-8"`, editable in the CF Admin panel). `BrowserFilter` sets
  `Content-Type: text/html; charset=<defaultCharset>`; `FORM`/`URL` scope
  charsets also default to it.

## 5. Encoding precedence summary

```
UTF-8 BOM / UTF-16(BE|LE) BOM   -> mandatory, cannot be overridden (error)
otherwise ICU detect (conf>=100)-> overrideable
otherwise platform default      -> overrideable
cfprocessingdirective pageEncoding -> overrides by full re-read+re-parse
```

## 6. Mapping to WebStrada

Current state of our engine:

- `compiler::compile` always parses the source with
  `TEXTPARSER_ENCODING_LATIN1` (`src/compiler.cpp:7239`).
- textparser already detects and **strips** BOMs itself and switches
  `text_format` accordingly (`/usr/include/textparser.h`; `textparser.c` around
  `BOM_UTF_8`/`BOM_UTF_16_LE`/`BOM_UTF_32_LE`); with no BOM it uses the passed
  default (`textparser_encoding` enum:
  `TEXTPARSER_ENCODING_LATIN1`, `_UTF_8`, `_UNICODE`, `_UTF_16`, `_UTF_32`).
- Internal strings are UTF-8 (codepoints stored as UTF-8 bytes). Charset
  conversion helpers already exist for output
  (`responseCharsetCanonical`, `codepointsToBytes`, `bytesToText` in
  `src/cf8.cpp`) and for `CharsetEncode`/`CharsetDecode`.
- `config::defaultOutputCharset = "UTF-8"` (`src/cf8.cpp:2019`) mirrors CF's
  admin-panel default for response output; there is no input-encoding config yet.

To implement `pageEncoding` we need an **input-encoding step** before tokenizing:
BOM (handled by textparser) else a configurable default (UTF-8 to match CF's
admin default), plus a compile-time pass that:

1. recognizes a literal `pageEncoding` value in a leading `cfprocessingdirective`
   (also inside `cfcomponent`),
2. validates the charset name -> error
   `The specified page encoding, {encoding}, is not supported.`,
3. if it conflicts with a detected BOM -> error
   `Cannot use the charset {encoding} because the file has a Byte Order Mark indicating it uses the charset {BOM encoding}.`,
4. otherwise re-decodes the raw file bytes with the requested charset and
   re-tokenizes the page,
5. strips the `pageencoding` attribute so the tag only handles
   `suppressWhitespace` at runtime.
