# Tag Name: `cfcalendar`

## Description
Puts an interactive Macromedia Flash format calendar in an HTML
 or Flash form. Not supported in XML format forms. The calendar
 lets a user select a date for submission as a form variable.

## Syntax
```cfml
<cfcalendar name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of the calendar.

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The vertical dimension of the calendar specified in pixels.

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The horizontal dimension of the calendar specified in pixels.

### Attribute: `selecteddate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The date that is initially selected. It is highlighted in a
 color determined by the form skin. Must be in mm/dd/yyyy
 or dd/mm/yyyy format, depending on the current locale.
 (Use the setlocale tag to set the locale, if necessary.)

### Attribute: `startrange`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The start of a range of dates that are disabled. Users
 cannot select dates from this date through the date
 specified by the endRange attribute.

### Attribute: `endrange`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The end of a range of dates that are disabled. Users
 cannot select dates from the date specified by the
 startRange attribute through this date.

### Attribute: `disabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Disables all user input, making the control read only.
 To disable input, specify disabled without an attribute
 or disabled="true". To enable input, omit the attribute
 or specify disabled="false".
 Default is: false

### Attribute: `mask`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `MM/DD/YYYY`
- **Description**: A pattern that specifies the format of the submitted date.
 Mask characters are:
 - D = day, can use 0-2 mask characters
 - M = month, can use 0-4 mask characters
 - Y = year, can use 0, 2, or 4 characters
 - E = day in week, can use 0-4 characters
 - Any other character = put the character in the specified location
 Default is: MM/DD/YYYY

### Attribute: `firstdayofweek`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Integer in the range 0-6 specifying the first day of the
 week in the calendar, 0 indicates Sunday, 6 indicates Saturday.
 Default is: 0

### Attribute: `daynames`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `S,M,T,W,Th,F,S`
- **Description**: A comma-delimited list that sets the names of the
 weekdays displayed in the calendar. Sunday is the
 first day and the rest of the weekday names follow in
 the normal order.
 Default is: S,M,T,W,Th,F,S

### Attribute: `monthnames`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `January,February,March,April,May,June,July,August,September,October,November,December`
- **Description**: A comma-delimited list of the month names that are
 displayed at the top of the calendar.

### Attribute: `enabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Specifying whether the control is enabled. A
 disabled control appears in light gray. This is the inverse
 of the disabled attribute.

### Attribute: `visible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Specifying whether to show the control. Space
 that would be occupied by an invisible control is blank.

### Attribute: `tooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: Text to display when the mouse pointer hovers
 over the control.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: Actionscript style or styles to apply to the calendar.
 Default is: haloGreen

### Attribute: `onchange`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: ActionScript that runs when the user selects a
 date.

### Attribute: `onblur`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: ActionScript that runs when the user selects a
 date.

### Attribute: `onfocus`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: ActionScript that runs when the user selects a
 date.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

