# Function Name: `XmlTransform`

## Description
Applies an Extensible Stylesheet Language Transformation (XSLT)
 to an XML document object that is represented as a string
 variable. An XSLT converts an XML document to another format
 or representation by applying an Extensible Stylesheet
 Language (XSL) stylesheet to it.

## Return Type
`string`

## Syntax
```cfml
xmlTransform(xml, xsl [, parameters])
```

## Arguments

### Argument: `xml`
- **Type**: `xml`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An XML document in string format, or an XML document object.

### Argument: `xsl`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: XSLT transformation to apply; can be any of the following:
 - A string containing XSL text.
 - The name of an XSTLT file. Relative paths start at
 the directory containing the current CFML page.
 - The URL of an XSLT file; valid protocol identifiers
 include http, https, ftp, and file. Relative paths start
 at the directory containing the current CFML page.

### Argument: `parameters`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A structure containing XSL template parameter name-value
 pairs to use in transforming the document. The XSL transform
 defined in the xslString parameter uses these parameter values
 in processing the xml.

## Limitations and Other Info

- **Type Requirement**: Operates on XML document objects or XML markup strings.
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

