# Tag Name: `cfmapitem`

## Description
This tag creates markers on the map.

## Syntax
```cfml
<cfmapitem>
```

## Attributes / Variants

### Attribute: `longitude`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The longitude value for the marker, in degrees.

### Attribute: `address`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The address of the location to set the map marker.

### Attribute: `tip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A short description of the marker location that appears as a tool tip.

### Attribute: `latitude`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The latitude value for the marker, in degrees.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the map.

### Attribute: `showMarkerWindow`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: When true, the marker infowindow is shown. By default, this is false. This is inherited by all cfmapitem tags.

### Attribute: `markerWindowContent`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Static inner HTML markup to be displayed in the infowindow opened when the marker is clicked. This is mutually exclusive with the markerbind attribute

### Attribute: `markerIcon`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Location of an image file to use as the marker icon. The attributes markericon and markercolor are mutually exclusive.

### Attribute: `markerColor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Indicates the color of the marker.

## Limitations

- **Must be nested inside**: `cfmap`
- **Must not be nested inside**: *None*

