<cfset chk_s = StructNew() />
<cfset chk_num = 42 />
<cfset chk_bool = true />
<cfset chk_emptyStr = "" />
<cfset chk_str = "hello" />
<cfset chk_float = 3.14 />
<cfset chk_num_str = "12.34" />
<cfset chk_abc = "abc" />

<cfoutput>
IsStruct_s:#IsStruct(chk_s)#|IsStruct_num:#IsStruct(chk_num)#
IsBoolean_t:#IsBoolean(chk_bool)#|IsBoolean_y:#IsBoolean("yes")#|IsBoolean_n:#IsBoolean(1)#|IsBoolean_str:#IsBoolean(chk_str)#
IsNumeric_42:#IsNumeric(chk_num)#|IsNumeric_float:#IsNumeric(chk_float)#|IsNumeric_str:#IsNumeric(chk_num_str)#|IsNumeric_abc:#IsNumeric(chk_abc)#
LSIsNumeric_42:#LSIsNumeric(chk_num)#|LSIsNumeric_str:#LSIsNumeric(chk_num_str)#|LSIsNumeric_abc:#LSIsNumeric(chk_abc)#
IsSimpleValue_42:#IsSimpleValue(chk_num)#|IsSimpleValue_s:#IsSimpleValue(chk_s)#
IsObject_s:#IsObject(chk_s)#|IsObject_num:#IsObject(chk_num)#
IsNull_num:#IsNull(chk_num)#
IsBinary_str:#IsBinary(chk_str)#
IsClosure_str:#IsClosure(chk_str)#
IsCustomFunction_str:#IsCustomFunction(chk_str)#
IsImage_str:#IsImage(chk_str)#
IsFileObject_str:#IsFileObject(chk_str)#
IsQuery_str:#IsQuery(chk_str)#
</cfoutput>
