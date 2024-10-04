<!--- FileWrite(file, string) appends a trailing newline like CF (was BUGS.md
     "FileWrite() does not append a newline"). CF: FileWrite(path,"aaaa") yields
     a 5-byte file, so Len(FileRead(path)) is 5 (a 4-char write plus the \n).
     Use GetTempDirectory() so both the CF server and the CLI resolve a writable
     path. --->
<cfset fpath = "#GetTempDirectory()#webstrada_fw_nl.txt">
<cfscript>
  FileWrite(fpath, "aaaa");
  n = Len(FileRead(fpath));
  FileWrite(fpath, "abcde");
  n2 = Len(FileRead(fpath));
</cfscript>
<cfoutput>#n#:#n2#</cfoutput>
