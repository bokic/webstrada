# Tag Name: `cfinvokeargument`

## Description
Passes the name and value of an argument to a component method or a web service method. This tag is used inside of the cfinvoke tag.

## Syntax
```cfml
<cfinvokeargument name="" value="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The argument name

### Attribute: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The argument value

### Attribute: `omit`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Enables you to omit a parameter when invoking a web service.
 It is an error to specify omit="true" if the cfinvoke
 webservice attribute is not specified.
 - true: omit this parameter when invoking a web service.
 - false: do not omit this parameter when invoking a web service.

## Limitations

- **Must be nested inside**: `cfinvoke`
- **Must not be nested inside**: *None*

