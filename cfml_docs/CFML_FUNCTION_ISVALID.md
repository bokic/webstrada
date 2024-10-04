# Function Name: `IsValid`

## Description
Tests whether a value meets a validation or data type rule.

## Return Type
`boolean`

## Syntax
```cfml
isValid(type, value, min, max, pattern)
```

## Arguments

### Argument: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The valid format for the data.

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The value to test.

### Argument: `min`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The minimum valid value; used only for range validation.

### Argument: `max`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The maximum valid value; used only for range validation.

### Argument: `pattern`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A regular expression that the parameter must match;
 used only for regex or regular_expression validation.

## Limitations and Other Info

- **Related Functions**: `isarray`, `issimplevalue`, `isnumeric`, `isboolean`, `isdate`, `cfparam`
- **Coldfusion Support**: Minimum version: `7`. Notes: CF8+ - added component as a type option.
CF11+ - no longer allows currency symbols at the start and commas inside a number. Can be reverted to legacy mode by setting this.strictNumberValidation = false in Application.cfc
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

