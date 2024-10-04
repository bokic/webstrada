# Tag Name: `cfpresenter`

## Description
Describes a presenter in a slide presentation. A slide presentation can have multiple presenters.
 The presenters must be referenced from the slides defined by the cfpresentationslide tag.

## Syntax
```cfml
<cfpresenter name="">
```

## Attributes / Variants

### Attribute: `biography`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specifies the biography of the presenter.

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specifies the name of the presenter.

### Attribute: `email`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name to use for the JavaScript proxy class.

### Attribute: `image`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the path for the presenter's image in JPEG format.
 The JPG file must be relative to the CFM page.

### Attribute: `logo`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the path for the presenter's logo or the logo of
 the presenters organization. The logo must be in JPEG format.
 The file must be relative to the CFM page.

### Attribute: `title`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the title of the presenter.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

