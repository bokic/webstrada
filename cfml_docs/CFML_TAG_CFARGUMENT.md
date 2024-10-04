# Tag Name: `cfargument`

## Description
Creates a parameter definition within a component definition. Defines a function argument. Used within a cffunction tag.

## Syntax
```cfml
<cfargument name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An argument name.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: a type name; data type of the argument.

### Attribute: `required`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `no`
- **Description**: Whether the parameter is required to execute the component method.

### Attribute: `default`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If no argument is passed, specifies a default argument value.

### Attribute: `displayname`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Meaningful only for CFC method parameters. A value to be displayed when using introspection to show information about the CFC.

### Attribute: `hint`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Meaningful only for CFC method parameters. Text to be displayed when using introspection to show information about the CFC. The hint attribute value follows the displayname attribute value in the parameter description line. This attribute can be useful for describing the purpose of the parameter.

## Limitations

- **Must be nested inside**: `cffunction`
- **Must not be nested inside**: *None*

