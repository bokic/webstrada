A
<cftimer type="inline" label="L1">BODY1</cftimer>
B
<cftimer type="comment" label="L2">BODY2</cftimer>
C
<cftimer type="debug" label="L3">BODY3</cftimer>
D
<cftimer label="L4">BODY4</cftimer>
E
<cftimer type="outline" label="L5">BODY5</cftimer>
F
<cftimer type="INLINE" label="L6">BODY6</cftimer>
G
<cfset t = "comment">
<cftimer type="#t#" label="L7">BODY7</cftimer>
H
<cftimer type="inline" label="L8">A<cftimer type="inline" label="L9">NESTED</cftimer>B</cftimer>
I
<cftimer type="inline" label="L10"/>
J
<cftry>
  <cftimer type="bogus" label="L11">BODY11</cftimer>
  <cfoutput>NOTCAUGHT|</cfoutput>
<cfcatch>
  <cfoutput>CAUGHT:#cfcatch.type#:#cfcatch.message#|</cfoutput>
</cfcatch>
</cftry>
K
<cfset x = "bogus">
<cftry>
  <cftimer type="#x#" label="L12">BODY12</cftimer>
  <cfoutput>NOTCAUGHT2|</cfoutput>
<cfcatch>
  <cfoutput>CAUGHT2:#cfcatch.type#:#cfcatch.message#:#cfcatch.detail#|</cfoutput>
</cfcatch>
</cftry>
L
<cftry>
  <cftimer type="">BODY13</cftimer>
  <cfoutput>NOTCAUGHT3|</cfoutput>
<cfcatch>
  <cfoutput>CAUGHT3:#cfcatch.detail#|</cfoutput>
</cfcatch>
</cftry>
M
<cfset y = 7>
<cftimer type="inline" label="L14"><cfoutput>#y#</cfoutput><cfset zz = 9></cftimer>
<cfoutput>[#zz#]</cfoutput>
N
<cfoutput>END</cfoutput>
