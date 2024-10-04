# Function Name: `ArrayNew`

## Description
Creates an array of 1-3 dimensions. Index array elements with square brackets: [ ]. CFML arrays expand dynamically as data is added.

## Return Type
`array`

## Syntax
```cfml
arrayNew(dimension [, isSynchronized])
```

## Arguments

### Argument: `dimension`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: `1`
- **Description**: 

### Argument: `isSynchronized`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: CF2016+ Lucee5.1+ When true creates a synchronized array. Unsynchronized arrays are not thread safe so they should not be used within shared scopes (application, session, etc). According to the CF2016 Performance whitepaper: Unsynchronized arrays are about 93% faster due to lock avoidance.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `structNew`, `queryNew`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**:

