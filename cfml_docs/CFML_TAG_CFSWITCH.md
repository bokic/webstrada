# Tag Name: `cfswitch`

## Description
Evaluates a passed expression and passes control to the cfcase tag that matches the expression result. You can, optionally, code a cfdefaultcase tag, which receives control if there is no matching cfcase tag value. Note the difference in the tag and script syntax when providing multiple values for a case.

## Syntax
```cfml
<cfswitch expression="">
```

## Attributes / Variants

### Attribute: `expression`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: CFML expression that yields a scalar value. CFML converts integers, real numbers, Booleans, and dates to numeric values. For example, True, 1, and 1.0 are all equal.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

