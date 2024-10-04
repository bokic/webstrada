# Tag Name: `cfmap`

## Description
Embeds a Google map within a ColdFusion web page

## Syntax
```cfml
<cfmap>
```

## Attributes / Variants

### Attribute: `width`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Map width, in pixels.

### Attribute: `centerlongitude`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The longitude value for the location, in degrees

### Attribute: `centeraddress`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The address of the location, which is set as the center of the map.

### Attribute: `continuouszoom`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to provide zoom control that enables smooth zooming for the map

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Type of the Google map

### Attribute: `title`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Title of the panel

### Attribute: `zoomcontrol`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to enable zoom control

### Attribute: `tip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A short description of the center location that appears as a tool tip.

### Attribute: `overview`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to add an Overview panel to the map

### Attribute: `hideborder`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to hide border for surrounding panel

### Attribute: `doubleclickzoom`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to enable double-click zoom

### Attribute: `key`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The API key of the map.

### Attribute: `collapsible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to provide a collapsible property for the surrounding panel

### Attribute: `zoomlevel`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the starting zoom value

### Attribute: `height`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Height of the map, in pixels

### Attribute: `centerlatitude`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The latitude value for the location, in degrees.

### Attribute: `onload`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function that runs after the map loads..

### Attribute: `typecontrol`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: What type of typecontrol to provide. Basic includes "map, satellite, hybrid" and Advanced includes "map, satellite, hybrid, earth, terrain"

### Attribute: `displayscale`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to enable scale control

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the map.

### Attribute: `scrollwheelzoom`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Whether to enable mouse wheel zooming control

### Attribute: `markerBind`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Uses a bind expression to fetch markup to be displayed in the infowindow opened when the marker is clicked. This is mutually exclusive with the markerwindowtext attribute. This is inherited by all cfmapitem tags

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
- **Description**: Location of the icon to use for the marker.

### Attribute: `markerColor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: marker color in hexadecimal value.

### Attribute: `onError`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The JavaScript function to run when there is a Google map API error.
The JavaScript function is passed with two parameters, Google map status code and error message.

### Attribute: `showCenterMarker`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Whether to display the marker icon that identifies the map center

### Attribute: `showScale`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Whether to show scale control

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

