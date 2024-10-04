# Function Name: `GetBaseTagList`

## Description
Gets a comma-delimited list of uppercase ancestor tag names, as a string. The first list element is the current tag. If the current tag is nested, the next element is the parent tag. If the function is called for a top-level tag, it returns an empty string.

## Return Type
`string`

## Syntax
```cfml
getBaseTagList()
```

## Arguments

### Argument: `caller`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Adobe only. Not aliased in Lucee

## Limitations and Other Info

- **Related Functions**: `getBaseTagData`
- **Coldfusion Support**: Notes: ACF behavavior is different from Lucee.
- **Lucee Support**: Notes: params:'delimiter' with default: ','.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

