# Function Name: `SessionRotate`

## Description
Creates a new session (using new session ids) and copies session scope into this new session, then invalidates the old session. Used after a valid login to prevent session fixation.

## Return Type
`void`

## Syntax
```cfml
sessionRotate()
```

## Arguments

This function does not take any arguments.

## Limitations and Other Info

- **Related Functions**: `sessioninvalidate`
- **Coldfusion Support**: Minimum version: `10`. Notes: Does not rotate jsessionid when JEE sessions are enabled, only works with ColdFusion sessions (CFID,CFTOKEN).
- **Lucee Support**:
- **Railo Support**: Minimum version: `4`.
- **Boxlang Support**: Minimum version: `1.0.0`.

