# Function Name: `GetBaseTagData`

## Description
Used within a custom tag. Finds calling (ancestor) tag by name and accesses its data.

## Return Type
`any`

## Syntax
```cfml
getBaseTagData(tagname [, level])
```

## Arguments

### Argument: `tagname`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specify the parent tag name, starting with CF_.

### Argument: `level`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: Specify the nth-level ancestor to retrieve variables from.

## Limitations and Other Info

- **Related Functions**: `getBaseTagList`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

