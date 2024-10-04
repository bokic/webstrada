# Function Name: `XmlElemNew`

## Description
Creates an XML document object element

## Return Type
`xml`

## Syntax
```cfml
xmlElemNew(xmlobj [, namespace], childname)
```

## Arguments

### Argument: `xmlobj`
- **Type**: `xml`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of an XML object. An XML document or an element.

### Argument: `namespace`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: URI of the namespace to which this element belongs.

### Argument: `childname`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of the element to create. This element becomes a
 child element of xmlObj in the tree.

## Limitations and Other Info

- **Type Requirement**: Operates on XML document objects or XML markup strings.
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

