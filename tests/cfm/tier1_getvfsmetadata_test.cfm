<!--- Tier-1: GetVFSMetaData + ram variations + invalid-type error (verified against CF 2025). --->
<cfoutput>
<cfset vfs = GetVFSMetaData("ram")>
1:[#vfs.enabled#,#vfs.limit#,#vfs.used#,#vfs.free#]|
<cfset vfs_caps = GetVFSMetaData("RAM")>
2:[#vfs_caps.enabled#]|
<cfset vfs_colon = GetVFSMetaData("ram:")>
3:[#vfs_colon.enabled#]|
<cfset vfs_uri = GetVFSMetaData("ram:///")>
4:[#vfs_uri.enabled#]|
5:<cftry><cfset vfs_bad = GetVFSMetaData("bogus")><cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
</cfoutput>
