# Function Name: `EntityLoadByExample`

## Description
Loads and returns an array of objects that match the `sampleEntity`.

## Return Type
`any`

## Syntax
```cfml
entityLoadByExample(sampleEntity [, unique, matchCriteria])
```

## Arguments

### Argument: `sampleEntity`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: No Help Available

### Argument: `unique`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: When true a single entity is returned, otherwise an array is returned.
If you are sure only one record exists for the `Filter`, then you can specify `unique=true` to return a single entity instead of an array. If you set `unique=true` and multiple records are returned, then an exception occurs.

### Argument: `matchCriteria`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: No Help Available

## Limitations and Other Info

- **Related Functions**: `entityLoadByPK`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

