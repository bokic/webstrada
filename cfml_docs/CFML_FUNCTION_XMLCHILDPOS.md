# Function Name: `XmlChildPos`

## Description
Gets the position of a child element within an XML document
 object.
 The position, in an XmlChildren array, of the Nth child that
 has the specified name.

## Return Type
`numeric`

## Syntax
```cfml
xmlChildPos(elem, childname, n)
```

## Arguments

### Argument: `elem`
- **Type**: `xml`
- **Required**: Required
- **Default Value**: *None*
- **Description**: XML element within which to search

### Argument: `childname`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: XML child element for which to search

### Argument: `n`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Index of XML child element for which to search

## Limitations and Other Info

- **Type Requirement**: Operates on XML document objects or XML markup strings.
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

