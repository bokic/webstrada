# Function Name: `IsAuthenticated`

## Description
CFML function IsAuthenticated.

## Return Type
`any`

## Syntax
```cfml
IsAuthenticated()
```

## Arguments

This function does not take any arguments.

## Limitations and Other Info

Obsolete security function from pre-MX ColdFusion Advanced Security. In modern Adobe ColdFusion (CF 2021/2025), calling this function results in `Variable ISAUTHENTICATED is undefined.` WebStrada reproduces this behavior, and user-defined functions (UDFs) with this name are allowed.

