# Function Name: `StructGet`

## Description
Returns a value in a structure or a structure in the specified path.

## Return Type
`any`

## Syntax
```cfml
structGet(path)
```

## Arguments

### Argument: `path`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Pathname of variable that contains structure or array from which CFML retrieves the value. If there is no structure or array present in the path, this function creates structures or arrays to make it a valid variable path.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

