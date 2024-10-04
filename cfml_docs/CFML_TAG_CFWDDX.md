# Tag Name: `cfwddx`

## Description
Serializes and deserializes CFML data structures to the
 XML-based WDDX format. The WDDX is an XML vocabulary for
 describing complex data structures in a standard, generic way.
 Implementing it lets you use the HTTP protocol to such
 information among application server platforms, application
 servers, and browsers.

 This tag generates JavaScript statements to instantiate
 JavaScript objects equivalent to the contents of a WDDX packet
 or CFML data structure. Interoperates with Unicode.

## Syntax
```cfml
<cfwddx action="cfml2wddx" input="">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: cfml2wddx: serialize CFML to WDDX
 wddx2cfml: deserialize WDDX to CFML
 cfml2js: serialize CFML to JavaScript
 wddx2js: deserialize WDDX to JavaScript

### Attribute: `input`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A value to process

### Attribute: `output`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of variable for output. If action = "WDDX2JS" or
 "CFML2JS", and this attribute is omitted, result is output
 in HTML stream.

### Attribute: `toplevelvariable`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of top-level JavaScript object created by
 deserialization. The object is an instance of the
 WddxRecordset object.

### Attribute: `usetimezoneinfo`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Whether to output time-zone information when serializing
 CFML to WDDX.
 - Yes: the hour-minute offset, represented in ISO8601
 format, is output.
 - No: the local time is output.

### Attribute: `validate`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Applies if action = "wddx2cfml" or "wddx2js".
 - Yes: validates WDDX input with an XML parser using
 WDDX DTD. If parser processes input without error,
 packet is deserialized. Otherwise, an error is
 thrown.
 - No: no input validation

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

