# Function Name: `GetProfileSections`

## Description
Gets all the sections of an initialization file.
Returns a struct whose format is as follows:
- Each initialization file section name is a key in the struct
- Each list of entries in a section of an initialization file is a value in the struct

## Return Type
`struct`

## Syntax
```cfml
getProfileSections(path [,encoding])
```

## Arguments

### Argument: `path`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Absolute path of initialization file

### Argument: `encoding`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF11+ Encoding of the initialization (ini) file

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `6`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-ini` module.

