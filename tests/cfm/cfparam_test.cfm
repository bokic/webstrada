<!--- Byte-verified against CF 2025. cfparam: find-or-default + type validation.
Errors are wrapped in cftry/cfcatch so the server does not abort the page. --->
<cfparam name="a" default="hello"><cfparam name="b" type="numeric" default="42"><cfparam name="c" type="boolean" default="true"><cfparam name="d" type="integer" default="12"><cfparam name="e" type="string" default="abc"><cfoutput>D1=#a#|#b#|#c#|#d#|#e#|</cfoutput><cfset existing = "keep"><cfparam name="existing"><cfoutput>D2=#existing#|</cfoutput>
<cfset existing2 = "keep2"><cfparam name="existing2" type="string"><cfoutput>D3=#existing2#|</cfoutput>
<cfparam name="url.param1" default="u1"><cfoutput>D4=#url.param1#|</cfoutput>
<cfparam name="deep.a.b.c" default="deepval"><cfoutput>D5=#deep.a.b.c#|</cfoutput>
<cfset arr = ArrayNew(1)><cfparam name="arr[2]" default="second"><cfoutput>D6=#arr[2]#:#ArrayLen(arr)#|</cfoutput>
<cfparam name="newArr[3]" default="third"><cfoutput>D7=#newArr[3]#:#ArrayLen(newArr)#|</cfoutput>
<cfparam name="newStruct.key" default="sv"><cfoutput>D8=#newStruct.key#|</cfoutput>
<cfset sideCount = 0><cffunction name="sideBump"><cfset variables.sideCount = variables.sideCount + 1><cfreturn "D"></cffunction>
<cfset sideVar = "already"><cfparam name="sideVar" default="#sideBump()#"><cfoutput>D9=#sideVar#:#sideCount#|</cfoutput>
<cftry><cfparam name="pv1" type="numeric" default="123.45"><cfoutput>T1=#pv1#|</cfoutput><cfcatch><cfoutput>T1E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv2" type="email" default="a@b.com"><cfoutput>T2=#pv2#|</cfoutput><cfcatch><cfoutput>T2E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv3" type="guid" default="AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE"><cfoutput>T3=#pv3#|</cfoutput><cfcatch><cfoutput>T3E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv4" type="uuid" default="AAAAAAAA-BBBB-CCCC-DDDDEEEEEEEEEEEE"><cfoutput>T4=#pv4#|</cfoutput><cfcatch><cfoutput>T4E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv5" type="ssn" default="123-45-6789"><cfoutput>T5=#pv5#|</cfoutput><cfcatch><cfoutput>T5E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv6" type="telephone" default="555-234-4567"><cfoutput>T6=#pv6#|</cfoutput><cfcatch><cfoutput>T6E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv7" type="zipcode" default="12345"><cfoutput>T7=#pv7#|</cfoutput><cfcatch><cfoutput>T7E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv8" type="USdate" default="1/2/2025"><cfoutput>T8=#pv8#|</cfoutput><cfcatch><cfoutput>T8E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv9" type="variableName" default="foo_bar2"><cfoutput>T9=#pv9#|</cfoutput><cfcatch><cfoutput>T9E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv10" type="url" default="http://www.adobe.com"><cfoutput>T10=#pv10#|</cfoutput><cfcatch><cfoutput>T10E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv11" type="creditcard" default="4111111111111111"><cfoutput>T11=#pv11#|</cfoutput><cfcatch><cfoutput>T11E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv12" type="eurodate" default="1/2/2025"><cfoutput>T12=#pv12#|</cfoutput><cfcatch><cfoutput>T12E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv13" type="time" default="12:30:45"><cfoutput>T13=#pv13#|</cfoutput><cfcatch><cfoutput>T13E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv14" type="date" default="1/2/2025"><cfoutput>T14=#pv14#|</cfoutput><cfcatch><cfoutput>T14E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv15" type="float" default="1.5"><cfoutput>T15=#pv15#|</cfoutput><cfcatch><cfoutput>T15E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv16" type="numeric_legacy" default="123"><cfoutput>T16=#pv16#|</cfoutput><cfcatch><cfoutput>T16E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfset pv17 = ArrayNew(1)><cfparam name="pv17" type="array"><cfoutput>T17=#IsArray(pv17)#|</cfoutput><cfcatch><cfoutput>T17E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfset pv18 = StructNew()><cfparam name="pv18" type="struct"><cfoutput>T18=#IsStruct(pv18)#|</cfoutput><cfcatch><cfoutput>T18E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv19" type="boolean" default="false"><cfoutput>T19=#pv19#:#IsBoolean(pv19)#|</cfoutput><cfcatch><cfoutput>T19E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv20" type="range" default="5" min="1" max="10"><cfoutput>T20=#pv20#|</cfoutput><cfcatch><cfoutput>T20E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfparam name="pv21" type="regex" default="123" pattern="^[0-9]+$"><cfoutput>T21=#pv21#|</cfoutput><cfcatch><cfoutput>T21E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR1:</cfoutput>
<cftry><cfparam name="missing1"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR2:</cfoutput>
<cftry><cfparam name="missingMixed"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR3:</cfoutput>
<cftry><cfparam name="missingType" type="numeric"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR4:</cfoutput>
<cftry><cfparam name="badNum" type="numeric" default="abc"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR5:</cfoutput>
<cftry><cfparam name="badInt" type="integer" default="1.5"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR6:</cfoutput>
<cftry><cfparam name="badBool" type="boolean" default="xyz"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR7:</cfoutput>
<cftry><cfparam name="badEmail" type="email" default="abc"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR8:</cfoutput>
<cftry><cfparam name="badGuid" type="guid" default="123"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR9:</cfoutput>
<cftry><cfparam name="badUuid" type="uuid" default="123"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR10:</cfoutput>
<cftry><cfparam name="badSsn" type="ssn" default="abc"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR11:</cfoutput>
<cftry><cfparam name="badTel" type="telephone" default="555-123-4567"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR12:</cfoutput>
<cftry><cfparam name="badZip" type="zipcode" default="abc"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR13:</cfoutput>
<cftry><cfparam name="badUsDate" type="USdate" default="32/1/2025"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR14:</cfoutput>
<cftry><cfparam name="badVarName" type="variableName" default="1foo"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR15:</cfoutput>
<cftry><cfparam name="badUrl" type="url" default="notaurl"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR16:</cfoutput>
<cftry><cfparam name="badCc" type="creditcard" default="1234"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR17:</cfoutput>
<cftry><cfparam name="badEuro" type="eurodate" default="32/32/2025"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR18:</cfoutput>
<cftry><cfparam name="badTime" type="time" default="25:00:00"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR19:</cfoutput>
<cftry><cfparam name="badType" type="bogus" default="x"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR20:</cfoutput>
<cftry><cfparam name="badNumTrue" type="numeric" default="true"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR21:</cfoutput>
<cftry><cfparam name="badDt" type="datetimeobject" default="1/1/2025"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR22:</cfoutput>
<cftry><cfparam name="rHi" type="range" default="15" min="1" max="10"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR23:</cfoutput>
<cftry><cfparam name="rLo" type="range" default="0" min="1" max="10"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR24:</cfoutput>
<cftry><cfparam name="rNoBounds" type="range" default="5"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR25:</cfoutput>
<cftry><cfparam name="regNoMatch" type="regex" default="abc123" pattern="[0-9]+"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR26:</cfoutput>
<cftry><cfparam name="regNoPattern" type="regex" default="abc"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR27:</cfoutput>
<cftry><cfset mlv = "abcdef"><cfparam name="mlv" type="string" maxlength="5"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR28:</cfoutput>
<cftry><cfset arrBad = ArrayNew(1)><cfparam name="arrBad" type="string"><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfoutput>|ERR29:</cfoutput>
<cftry><cfparam name="bodyParam" default="bp"><cfset bodyZ = 99>SKIPPED</cfparam><cfoutput>#IsDefined("bodyZ")#</cfoutput><cfcatch><cfoutput>#cfcatch.type#|#cfcatch.message#</cfoutput></cfcatch></cftry>
<cfoutput>|END</cfoutput>
