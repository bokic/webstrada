# Function Name: `XmlSearch`

## Description
Get XML values according to given xPath query

## Return Type
`array`

## Syntax
```cfml
xmlSearch(xmlNode, xpath [, params])
```

## Arguments

### Argument: `xmlNode`
- **Type**: `xml`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An XML document object

### Argument: `xpath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: xPath expression

### Argument: `params`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ A struct with key value pairs to be used a variables within the xPath Expression

## Limitations and Other Info

- **Type Requirement**: Operates on XML document objects or XML markup strings.
- **Related Functions**: `xmlParse`
- **Coldfusion Support**: Minimum version: `6`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

