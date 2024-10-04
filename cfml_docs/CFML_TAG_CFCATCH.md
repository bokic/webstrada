# Tag Name: `cfcatch`

## Description
Used inside a cftry tag. Together, they catch and process
 exceptions in CFML pages. Exceptions are events that
 disrupt the normal flow of instructions in a CFML page,
 such as failed database operations, missing include files, and
 developer-specified events.

## Syntax
```cfml
<cfcatch>
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Variable name for cfcatch expression.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `any`
- **Description**: `application`: catches application exceptions
`database`: catches database exceptions
`template`: catches ColdFusion page exceptions
`security`: catches security exceptions
`object`: catches object exceptions
`missingInclude`: catches missing include file exceptions
`expression`: catches expression exceptions
`lock`: catches lock exceptions
`custom_type`: catches the specified custom exception type that is defined in a cfthrow tag
 `java.lang.Exception`: catches Java object exceptions
 `searchengine`: catches Verity search engine exceptions
 `any`: catches all exception types

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

