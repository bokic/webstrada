# Function Name: `ArrayInsertAt`

## Description
Inserts a value at the specified position in the array. If the element is inserted before the end of the array, ColdFusion shifts the positions of all elements with a higher index to make room.

## Return Type
`boolean`

## Syntax
```cfml
arrayInsertAt(array, position, value)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The array which will have the new element inserted.

### Argument: `position`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The numerical index in the array where the new element will be inserted.
 Must be less than or equal to the length of the array.
 Remember ColdFusion arrays start at 1 not 0.

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The new element to insert.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayappend`, `arrayprepend`, `arraydelete`, `arraydeleteat`, `arrayset`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

