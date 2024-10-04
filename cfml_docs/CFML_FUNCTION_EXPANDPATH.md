# Function Name: `ExpandPath`

## Description
Creates an absolute, platform-appropriate path that is equivalent to the value of 'path', appended to the base path. This function (despite its name) can accept an absolute or relative path in the 'path' attribute.

## Return Type
`string`

## Syntax
```cfml
expandPath(path)
```

## Arguments

### Argument: `path`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Relative or absolute directory reference or filename, within the current directory, (.\ and ..\) to convert to an absolute path. Can include forward or backward slashes.

## Limitations and Other Info

- **Related Functions**: `contractPath`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

