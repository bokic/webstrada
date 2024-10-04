<!--- CFKillBoard parser.cfm invalidMail guard regression test.
The app's parser.cfm only sets `victim` inside `<cfif invalidMail IS FALSE>`;
the validation block must not reference `victim` when invalidMail is TRUE,
otherwise garbage input throws "Variable VICTIM is undefined." instead of
showing "Killmail format is not valid". This replicates the fixed structure. --->
<cfscript>
function parseMail(text) {
	var killArray = ListToArray(text, "#Chr(13)##Chr(10)#");
	var SearchPosition = 1;
	var foundDate = FALSE;
	var invalidMail = FALSE;
	var lineToCheck = "";
	var victim = "";
	while (foundDate IS FALSE) {
		lineToCheck = Trim(killArray[SearchPosition]);
		if (Left(lineToCheck,4) LE Year(Now()) AND Left(lineToCheck,4) GE 2003) {
			foundDate = TRUE;
		}
		if (SearchPosition GE ArrayLen(killArray)) {
			foundDate = TRUE;
			invalidMail = TRUE;
		}
		SearchPosition = SearchPosition + 1;
	}
	if (invalidMail IS FALSE) {
		victim = ListToArray(killArray[SearchPosition], ":");
	}
	if (invalidMail IS FALSE) {
		if (victim[1] EQ "Victim") {
			return "valid-format";
		} else {
			return "Killmail format is not valid";
		}
	} else {
		return "Killmail format is not valid";
	}
}
</cfscript>
<cfoutput>
1:[#parseMail("ioljiojo")#]<br>
2:[#parseMail("random text without date")#]<br>
3:[#parseMail("2026.05.21 18:22#Chr(13)##Chr(10)#Victim: Someone")#]<br>
4:[#parseMail("2026.05.21 18:22#Chr(13)##Chr(10)#SomethingElse: Someone")#]<br>
5:[#parseMail("1999.01.01 00:00#Chr(13)##Chr(10)#Victim: Someone")#]<br>
6:[#parseMail("2027.01.01 00:00#Chr(13)##Chr(10)#Victim: Someone")#]
</cfoutput>
