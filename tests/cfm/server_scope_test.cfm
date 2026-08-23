<cfoutput>
coldfusion_defined:#isDefined("server.coldfusion")#|
appserver_defined:#isDefined("server.coldfusion.appserver")#|
productname_defined:#isDefined("server.coldfusion.productname")#|
productname:#server.coldfusion.productname#|
productlevel_defined:#isDefined("server.coldfusion.productlevel")#|
productlevel:#server.coldfusion.productlevel#|
productversion_defined:#isDefined("server.coldfusion.productversion")#|
productversion:#server.coldfusion.productversion#|
productversion_listfirst:#listFirst(server.coldfusion.productversion)#|
productversion_gte_8:#listFirst(server.coldfusion.productversion) gte 8#|
rootdir_defined:#isDefined("server.coldfusion.rootdir")#|
serialnumber_defined:#isDefined("server.coldfusion.serialnumber")#|
supportedlocales_defined:#isDefined("server.coldfusion.supportedlocales")#
</cfoutput>
