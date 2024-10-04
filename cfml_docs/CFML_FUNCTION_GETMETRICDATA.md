# Function Name: `GetMetricData`

## Description
Gets server performance metrics
 [mode - quickly]

## Return Type
`any`

## Syntax
```cfml
getMetricData(mode)
```

## Arguments

### Argument: `mode`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: perf_monitor - Returns internal data, in a structure.
 simple_load - Returns an integer value that is computed
 from the state of the server's internal
 queues. Indicates the overall server load.
 prev_req_time - Returns the time, in milliseconds, that it
 took the server to process the previous
 request.
 avg_req_time - Returns the average time, in milliseconds,
 that it takes the server to process a
 request.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `4.5`.
- **Lucee Support**:
- **Railo Support**:

