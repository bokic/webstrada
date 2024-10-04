# Function Name: `QuotedValueList`

## Description
 Gets the values of each record returned from an executed query.
 CFML does not evaluate the arguments

## Return Type
`string`

## Syntax
```cfml
quotedValueList(column [, delimiter])
```

## Arguments

### Argument: `column`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of an executed query and column. Separate query name
 and column name with a period.

### Argument: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: 

## Limitations and Other Info

- **Related Functions**: `ValueList`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Transpiled to `queryColumnData().map().toList( delimiter )` in BoxLang

