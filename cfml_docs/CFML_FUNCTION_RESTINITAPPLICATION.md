# Function Name: `RestInitApplication`

## Description
Scans all the CFCs in dirPath, and places those that are REST enabled at the serviceMapping URL. Requires the web admin password under Lucee.

## Return Type
`void`

## Syntax
```cfml
restInitApplication(dirPath,serviceMapping,default,password)
```

## Arguments

### Argument: `dirPath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The path to a folder of CFCs to scan. Should be a full file system path

### Argument: `serviceMapping`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The root of the exposed API, minus the server-wide prefix. E.g. to expose at '/rest/api/' you should set this to 'api'.

### Argument: `default`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If the mapping is a default mapping set this to true (Lucee only)

### Argument: `password`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The password for the web admin (Lucee only)

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:

