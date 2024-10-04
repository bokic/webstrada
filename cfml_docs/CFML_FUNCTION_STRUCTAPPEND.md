# Function Name: `StructAppend`

## Description
Appends one structure to another.

## Return Type
`boolean`

## Syntax
```cfml
structAppend(destStruct, sourceStruct [, overwriteFlag])
```

## Arguments

### Argument: `struct1`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Structure to which struct2 is appended.

### Argument: `struct2`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Structure that contains the data to append to struct1.

### Argument: `overwriteFlag`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Whether values in struct2 should overwrite corresponding values in
 struct1 or not.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Coldfusion Support**: Notes: CF6+ can be used on XML objects as well as structures.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

