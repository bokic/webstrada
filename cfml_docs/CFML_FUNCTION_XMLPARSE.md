# Function Name: `XmlParse`

## Description
Converts an XML document that is represented as a string
 variable into an XML document object.

## Return Type
`xml`

## Syntax
```cfml
xmlParse(xmlString [, caseSensitive] [, validator])
```

## Arguments

### Argument: `xmlString`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Any of the following:
 - A string containing XML text.
 - The name of an XML file.
 - The URL of an XML file; valid protocol identifiers
 include http, https, ftp, and file.

### Argument: `caseSensitive`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Maintains the case of document elements and attributes.
 Default: false

### Argument: `validator`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Any of the following:
 - The name of a Document Type Definition (DTD) or
 XML Schema file.
 - The URL of a DTD or Schema file; valid protocol
 identifiers include http, https, ftp, and file.
 - A string representation of a DTD or Schema.
 - An empty string; in this case, the XML file must
 contain an embedded DTD or Schema identifier, which
 is used to validate the document.

## Limitations and Other Info

- **Type Requirement**: Operates on XML document objects or XML markup strings.
- **Related Functions**: `xmlNew`, `serializeXML`, `deserializeXML`, `isXMLDoc`, `encodeForXML`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

