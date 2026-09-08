# Failed Unit Tests

After rerunning the full unit suite with access to `/var/log/webstrada/`, 12
host-level failures remain. The trace/logging failure seen in the restricted
environment is not included here because `JitExpressionTest.Tier2TraceAndAjax`
passes when the log directory is accessible.

| Test | Reason for failure |
| --- | --- |
| `ArrayStructLiteralTest.UnsupportedArrayOutputThrows` | Direct array output is accepted/rendered, but Adobe ColdFusion throws an exception for unsupported complex output. |
| `CfHttpTest.GetAsBinaryNoStoresByteArrayOutputStream` | The local HTTP fixture returns incorrect body bytes instead of the expected binary `0x00..0xFF` payload. |
| `CfHttpTest.GetAsBinaryYesStoresBinary` | The same local HTTP fixture mismatch prevents the expected binary response content or metadata from being received. |
| `CfQueryTest.DbLayerDumpsOperationsToStdout` | Expected SQLite open/close diagnostic lines are absent from captured stdout. |
| `CfQueryTest.DsnFileCreatedNextToConfiguredDir` | The expected DSN SQLite file is not created in the configured directory; the failure is order-dependent and passes in isolation. |
| `JitExpressionTest.MemberChainAfterBracketIndex` | A member chain following bracket indexing resolves its base incorrectly and reports variable `B` as undefined. |
| `JitExpressionTest.Tier2NumberFormat` | `NumberFormat` receives an empty or null value where Adobe ColdFusion accepts the tested argument form. |
| `JitExpressionTest.UnimplementedAndUnknownTags` | The test expects `<cfmail>` to throw, but it is now intentionally implemented as a non-throwing logging stub. |
| `LocaleTest.LSIsDateUsesLocale` | German locale-specific date forms are rejected, and one tested call uses an unsupported argument count. |
| `StructScopeFunctionsTest.ScopePassedDirectly` | The `variables` scope contains one extra enumerated key; `StructCount(variables)` returns `2` instead of `1`. |
| `StructScopeFunctionsTest.UdfRegisteredInVariables` | Registering a UDF leaves an extra key or state in `variables`; Adobe ColdFusion expects a count of `1`. |
| `UdfTest.ArgumentsVisibleKeysMatchCf` | The `arguments` scope exposes different named or positional keys, ordering, or counts than Adobe ColdFusion. |

The complete run is recorded in
[`tmp/full_unit_suite_elevated.log`](tmp/full_unit_suite_elevated.log).
