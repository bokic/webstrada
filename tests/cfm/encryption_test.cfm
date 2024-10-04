<cfset key = "0123456789ABCDEF0123456789ABCDEF">
<cfset iv = BinaryDecode(RepeatString("00",16),"Hex")>
<cfset eb = EncryptBinary(ToBinary(ToBase64("Hello")), key, "AES")>
<cfset db = DecryptBinary(eb, key, "AES")>
<cfset enc_cbc = Encrypt("Hello World", key, "AES/CBC/PKCS5Padding", "Base64", iv)>
<cfoutput>
hash_default=#Hash("Hello")#
hash_md5=#Hash("Hello","MD5")#
hash_sha1=#Hash("Hello","SHA-1")#
hash_sha256=#Hash("Hello","SHA-256")#
hash_sha384=#Hash("Hello","SHA-384")#
hash_sha512=#Hash("Hello","SHA-512")#
hash_empty=#Hash("")#
hash_iter1=#Hash("Hello","MD5","UTF-8",1)#
hash_upper_alg=#Hash("Hello","MD5")#
hmac_default=#Hmac("message","key")#
hmac_sha1=#Hmac("message","key","HmacSHA1")#
hmac_sha256=#Hmac("message","key","HmacSHA256")#
b64_str=#ToBase64("Hello World")#
b64_bin=#ToBase64(ToBinary("SGVsbG8="))#
is_binary=#IsBinary(ToBinary("SGVsbG8="))#
bin_len=#Len(ToBinary("SGVsbG8="))#
enc_hex=#BinaryEncode(ToBinary("SGVsbG8="),"Hex")#
enc_base64=#BinaryEncode(ToBinary("SGVsbG8="),"Base64")#
enc_base64url=#BinaryEncode(ToBinary("SGVsbG8="),"Base64URL")#
enc_aes=#Encrypt("Hello World", key, "AES", "Base64")#
dec_aes=#Decrypt(Encrypt("Hello World", key, "AES", "Base64"), key, "AES", "Base64")#
enc_aes_hex=#Encrypt("Hello World", key, "AES", "Hex")#
enc_cbc=#enc_cbc#
dec_cbc=#Decrypt(enc_cbc, key, "AES/CBC/PKCS5Padding", "Base64", iv)#
enc_des=#Encrypt("Hello World", "93Wn5ftSokM=", "DES", "Base64")#
enc_3des=#Encrypt("Hello World", key, "DESEDE", "Base64")#
enc_blowfish=#Encrypt("Hello World", "khJoMdQKRgOsHJEuRRY1Ng==", "BLOWFISH", "Base64")#
enc_uuro=#Decrypt(Encrypt("Hello World", key, "AES"), key, "AES")#
pbkdf1=#GeneratePBKDFKey("PBKDF2WithHmacSHA1","password","salt",1000,128)#
pbkdf2=#GeneratePBKDFKey("PBKDF2WithHmacSHA256","password","salt",1000,256)#
g3d=#Generate3DesKey("hello")#
encbin=#BinaryEncode(eb,"Base64")#
decbin=#BinaryEncode(db,"Base64")#
decbin_len=#Len(db)#
</cfoutput>
