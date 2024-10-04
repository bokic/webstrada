# Tag Name: `cfcase`

## Description
Used only inside the cfswitch tag body. Contains code to execute when the expression specified in the cfswitch tag has one or more specific values.  Note the difference in the tag and script syntax when providing multiple values for a case.

## Syntax
```cfml
<cfcase value="">
```

## Attributes / Variants

### Attribute: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The value or values that the expression attribute of the cfswitch tag must match. To specify multiple matching values, for tag syntax, separate the values with the delimiter character; for script syntax list each on the same line. The value or values must be simple constants or constant expressions, not variables.

### Attribute: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: Specifies the delimiter character or characters that separate multiple values to match. If you specify multiple delimiter characters, you can use any of them to separate the values to be matched. Used only for tag syntax.

## Limitations

- **Must be nested inside**: `cfswitch`
- **Must not be nested inside**: *None*

