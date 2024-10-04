<cfscript>
nl = Chr(10);
fileWrite(ExpandPath("t2_sps.ini"), "[sec1]" & nl & "k1=v1" & nl & "[sec2]" & nl & "k2=v2" & nl);
r = SetProfileString(ExpandPath("t2_sps.ini"), "sec1", "k1", "newval");
writeOutput("1:[" & r & "]");
r2 = SetProfileString(ExpandPath("t2_sps.ini"), "sec1", "k3", "v3");
r3 = SetProfileString(ExpandPath("t2_sps.ini"), "sec3", "k4", "v4");
writeOutput("2:[" & GetProfileString(ExpandPath("t2_sps.ini"), "sec1", "k1") & "]");
writeOutput("3:[" & GetProfileString(ExpandPath("t2_sps.ini"), "sec1", "k3") & "]");
writeOutput("4:[" & GetProfileString(ExpandPath("t2_sps.ini"), "sec3", "k4") & "]");
</cfscript>
