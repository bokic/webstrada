# Tag Name: `cfimapfilter`

## Description
Specifies filter parameters that control the actions of cfimap, get operations.

Two ways to use this tag: [name, value pair attributes] or [name, from, to ].


 name = "filter type"
 value = "filter value"

OR 
 

 name = "filter type"
 from = "date/time"
 to = "date/time"

## Syntax
```cfml
<cfimapfilter name="filter type" ... >
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: type of imap filter to use. TimeReceived and TimeSent must be used with the from/to attributes 

### Attribute: `from`
- **Type**: `date`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The start date or date/time combination of the range to use for filtering. Cannot be used with the value attribute. If you specify a from attribute without a to attribute, the filter selects for all entries on or after the specified date or time.
The value can be in any date/time format recognized by ColdFusion, but must correspond to a value that is appropriate for the filter type.

### Attribute: `to`
- **Type**: `date`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The end date or date/time combination for the range used for filtering. Cannot be used with the value attribute. If you specify a to attribute without a from attribute, the filter selects for all entries on or before the specified date or time.
The value can be in any date/time format recognized by ColdFusion, but must correspond to a value that is appropriate for the filter type.

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Filter Value

## Limitations

- **Must be nested inside**: `cfimap`
- **Must not be nested inside**: *None*

