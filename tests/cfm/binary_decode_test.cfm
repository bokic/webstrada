<cfset hexBin = BinaryDecode("48656C6C6F", "hex")>
<cfset hexLowerBin = BinaryDecode("48656c6c6f", "hex")>
<cfset hexEmptyBin = BinaryDecode("", "hex")>
<cfset hexMixedCaseEnc = BinaryDecode("48656C6C6F", "HeX")>
<cfset b64Bin = BinaryDecode("SGVsbG8=", "base64")>
<cfset b64EmptyBin = BinaryDecode("", "base64")>
<cfset b64UrlBin = BinaryDecode("SGVsbG8=", "base64URL")>
<cfset b64UrlEmptyBin = BinaryDecode("", "base64URL")>
<cfset uuBin = BinaryDecode("%2&5L;&\`" & Chr(10) & "`" & Chr(10), "UU")>
<cfset uuEmptyBin = BinaryDecode("", "UU")>
<cfoutput>
hex_is_binary:#IsBinary(hexBin)#
hex_lower_is_binary:#IsBinary(hexLowerBin)#
hex_empty_is_binary:#IsBinary(hexEmptyBin)#
hex_mixed_encoding_case_is_binary:#IsBinary(hexMixedCaseEnc)#
base64_is_binary:#IsBinary(b64Bin)#
base64_empty_is_binary:#IsBinary(b64EmptyBin)#
base64url_is_binary:#IsBinary(b64UrlBin)#
base64url_empty_is_binary:#IsBinary(b64UrlEmptyBin)#
uu_is_binary:#IsBinary(uuBin)#
uu_empty_is_binary:#IsBinary(uuEmptyBin)#
</cfoutput>
