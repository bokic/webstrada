<cfoutput>START;</cfoutput>
<cfset p = new components.typed_person()>
<cfset st = DeserializeJSON(SerializeJSON(p))>
<cfoutput>#st.age#|#st.active#|#st.score#|#st.title#|</cfoutput>
<cfset p.age = 25>
<cfset st2 = DeserializeJSON(SerializeJSON(p))>
<cfoutput>#st2.age#|#st2.active#|#st2.score#|#st2.title#|</cfoutput>
