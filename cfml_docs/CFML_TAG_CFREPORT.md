# Tag Name: `cfreport`

## Description
Used to do either of the following:
 - Execute a report definition created with the ColdFusion
 Report Builder, displaying it in PDF, FlashPaper, or Excel
 format. You can optionally save this report to a file.
 - Run a predefined Crystal Reports report. Applies only to
 Windows systems. Uses the CFCRYSTAL.exe file to generate
 reports. Sets parameters in the Crystal Reports engine
 according to its attribute values.

## Syntax
```cfml
<cfreport>
```

## Attributes / Variants

### Attribute: `template`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the path to the report definition file,
 relative to the web root.

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the output format.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the ColdFusion variable that will hold
 the report output. You cannot specify both name and
 filename.

### Attribute: `filename`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The filename to contain the report. You cannot
 specify both name and filename.

### Attribute: `query`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the query that contains input data for
 the report. If you omit this parameter, the report
 definition obtains data from the internal SQL or from
 cfreportparam items.

### Attribute: `overwrite`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specifies whether to overwrite files that have the
 same name as that specified in the filename attribute.
 Default: false

### Attribute: `encryption`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `none`
- **Description**: Specifies whether the output is encrypted. PDF format only.
 Default: none

### Attribute: `ownerpassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies an owner password. PDF format only.

### Attribute: `userpassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies a user password. PDF format only.

### Attribute: `permissions`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies one or more permissions. PDF format only.
 Separate multiple permissions with a comma.

### Attribute: `datasource`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of registered or native data source.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `standard`
- **Description**: * standard (not valid for Crystal Reports 8.0)
 * netscape
 * microsoft

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum time, in seconds, in which a connection must be
 made to a Crystal Report.

### Attribute: `report`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Report path. Store Crystal Reports files in the same
 directories as CFML page files.

### Attribute: `orderby`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Orders results according to your specifications.

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Username required for entry into database from which report
 is created. Overrides default settings for data source in
 CFML Administrator.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Password that corresponds to username required for database
 access. Overrides default settings for data source in
 CFML Administrator.

### Attribute: `formula`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: One or more named formulas. Terminate each formula with a
 semicolon. Use the format:

 formula = "formulaname1='formula1';formulaname2='formula2';"

 If you use a semicolon in a formula, you must escape it by
 typing it twice (;;). For example:

 formula = "Name1 = 'Val_1a;;Val_1b';Name2 = 'Val2';"

### Attribute: `resourceTimespan`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Life span of the resource directory. Used only with format=HTML

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Style in CSS format that overrides a style defined in the Report Builder report at run time.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

