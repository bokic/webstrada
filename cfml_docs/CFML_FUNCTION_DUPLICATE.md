# Function Name: `Duplicate`

## Description
Returns a clone (or deep copy) of an object or variable, leaving no reference to the original. Use this function to duplicate complex structures, such as nested structures and queries. When you duplicate a CFC instance, the entire CFC contents is copied, including the values of the variables in the `this` scope at the time you call the `Duplicate` function. Thereafter, the two CFC instances are independent, and changes to one copy, for example by calling one of its functions, have no effect on the other copy. Note: With this function, you cannot duplicate a COM, CORBA, or JAVA object returned from the cfobject tag or the CreateObject function. If an array element or structure field is a COM, CORBA, or JAVA object, you cannot duplicate the array or structure.

## Return Type
`any`

## Syntax
```cfml
duplicate(object)
```

## Arguments

### Argument: `object`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the object to duplicate.

### Argument: `deepcopy`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: Lucee Only. If set to `true` (default) the child elements are also cloned. If `false`, child elements retain a reference to their corresponding element in the original object. Note: deeply cloned elements that are not native Lucee objects (i.e. Java objects) may change data type when they can be converted to a native CFML object.

## Limitations and Other Info

- **Related Functions**: `structCopy`
- **Coldfusion Support**: Minimum version: `4.5`. Notes: CFMX allows this function to be used on XML objects. CF8 allows this to duplicate CFCs.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

