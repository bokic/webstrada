# Function Name: `ValueList`

## Description
Returns each value from a column of an executed query.
 CFML does not evaluate the arguments.
 A delimited list of the values of each record returned from an executed query column

## Return Type
`string`

## Syntax
```cfml
valueList(column [, delimiter])
```

## Arguments

### Argument: `column`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of an executed query and column. Separate query name and column name with a period.

### Argument: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: A delimiter character to separate column data items.
 Default: comma (,).

## Limitations and Other Info

- **Related Functions**: `valueArray`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Transplies to `queryColumnData().toList( delimiter )` in BoxLang

