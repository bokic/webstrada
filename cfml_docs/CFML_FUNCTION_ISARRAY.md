# Function Name: `IsArray`

## Description
 Determines whether a value is an array.
 True, if value is an array, a query column object, or XML.
 also number = Dimension; function tests whether the array has
 exactly this dimension

## Return Type
`boolean`

## Syntax
```cfml
isArray(value [, number])
```

## Arguments

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The array in which to check.

### Argument: `number`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Dimension of the array.

## Limitations and Other Info

- **Related Functions**: `isStruct`, `arrayIsEmpty`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

