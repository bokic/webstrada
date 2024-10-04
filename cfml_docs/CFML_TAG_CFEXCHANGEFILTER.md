# Tag Name: `cfexchangefilter`

## Description
Specifies the filter parameter for cfexchangemail, cfexchangecalendar, cfexchangetask, and cfexchangecontact, get operations.

## Syntax
```cfml
<cfexchangefilter name="allDayEvent">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The type of filter to use. (required)

### Attribute: `from`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The start date or date/time combination of the range to use for filtering.
 Cannot be used with the value attribute.
 If you specify a from attribute without a to attribute, the filter selects for
 all entries on or after the specified date or time.
 The value can be in any date/time format recognized by ColdFusion, but must
 correspond to a value that is appropriate for the filter type. (optional)

### Attribute: `to`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The end date or date/time combination for the range used for filtering.
 Cannot be used with the value attribute.
 If you specify a to attribute without a from attribute, the filter selects for
 all entries on or before the specified date or time.
 The value can be in any date/time format recognized by ColdFusion, but must
 correspond to a value that is appropriate for the filter type. (optional)

### Attribute: `value`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The filter value for all filters that do not take a date or time range.
 Cannot be used with the from and to attributes.
 If the name attribute requires this attribute, ColdFusion generates an error
 if it has an empty or null value. (optional)

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

