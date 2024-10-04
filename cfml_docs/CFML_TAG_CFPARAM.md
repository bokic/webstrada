# Tag Name: `cfparam`

## Description
Tests for a parameter's existence, tests its data type, and, if
 a default value is not assigned, optionally provides one.

## Syntax
```cfml
<cfparam name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of parameter to test (such as "client.email" or
 "cookie.backgroundColor"). If omitted, and if the
 parameter does not exist, an error is thrown.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `Any`
- **Description**: The valid format for the data; one of the following.
 * any: any type of value.
 * array: an array of values.
 * binary: a binary value.
 * boolean: a Boolean value: yes, no, true, false, or a number.
 * creditcard: a 13-16 digit number conforming to the mod10 algorithm.
 * date or time: a date-time value.
 * email: a valid e-mail address.
 * eurodate: a date-time value. Any date part must be in the format dd/mm/yy, The format can use /, -, or . characters as delimiters.
 * float or numeric: a numeric value.
 * guid: a Universally Unique Identifier of the form "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" where 'X' is a hexadecimal number.
 * numeric: a numeric value
 * integer: an integer.
 * query: a query object.
 * range: a numeric range, specified by the min and max attributes.
 * regex or regular_expression: matches input against pattern attribute.
 * ssn or social_security_number: a U.S. social security number.
 * string: a string value or single character.
 * struct: a structure.
 * telephone: a standard U.S. telephone number.
 * URL: an http, https, ftp, file, mailto, or news URL.
 * UUID: a ColdFusion Universally Unique Identifier, formatted 'XXXXXXXX-XXXX-XXXX-XXXXXXXXXXXXXXX', where 'X' is a hexadecimal number. See CreateUUID.
 * USdate: a U.S. date of the format mm/dd/yy, with 1-2 digit days and months, 1-4 digit years.
 * variableName: a string formatted according to ColdFusion variable naming conventions.
 * xml: XML objects and XML strings.
 * zipcode: U.S., 5- or 9-digit format ZIP codes.

### Attribute: `default`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Value to set parameter to if it does not exist. Any
 expression used for the default attribute is evaluated,
 even if the parameter exists.
 The result is not assigned if the parameter exists,
 but if the expression has side effects, they still occur.

### Attribute: `max`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The maximum valid value; used only for range validation.

### Attribute: `min`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The minimum valid value; used only for range validation.

### Attribute: `pattern`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A regular expression that the parameter must match;
 used only for regex or regular_expression validation.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

