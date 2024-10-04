# Tag Name: `cfdocumentitem`

## Description
Specifies action items for a PDF or FlashPaper document
 created by the cfdocument tag.

## Syntax
```cfml
<cfdocumentitem type="pagebreak">
```

## Attributes / Variants

### Attribute: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specifies the action:
 - pagebreak: start a new page at the location of the tag.
 - header: use the text between the <cfdocumentitem>
 and </cfdocumentitem> tags as the running header.
 - footer: use the text between the <cfdocumentitem>
 and </cfdocumentitem> tags as the running footer.

### Attribute: `evalAtPrint`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies whether to evaluate expressions inside cfdocumentitem tag at runtime.

## Limitations

- **Must be nested inside**: `cfdocument`
- **Must not be nested inside**: *None*

