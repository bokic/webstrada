<cfset rgb = ImageNew("", 32, 32, "rgb") />
<cfset argb = ImageNew("", 10, 20, "argb") />
<cfset gray = ImageNew("", 8, 16, "grayscale") />
<cfset def = ImageNew("", 20, 5) />
<cfset def2 = ImageNew("", 3, 7, "rgb") />
<cfset one = ImageNew("", 1, 1, "rgb") />
<cfset big = ImageNew("", 640, 480, "grayscale") />
<cfoutput>
#ImageGetWidth(rgb)#x#ImageGetHeight(rgb)#|
#ImageGetWidth(gray)#x#ImageGetHeight(gray)#|
#ImageGetWidth(def)#x#ImageGetHeight(def)#|
#ImageGetWidth(one)#x#ImageGetHeight(one)#|
#ImageGetWidth(big)#x#ImageGetHeight(big)#|
#ImageInfo(rgb).width#x#ImageInfo(rgb).height#|
#ImageInfo(gray).width#x#ImageInfo(gray).height#|
#ImageInfo(argb).width#x#ImageInfo(argb).height#|
#len(ImageInfo(rgb).source)#|
#SerializeJSON(ImageInfo(rgb).colormodel)#|
#SerializeJSON(ImageInfo(argb).colormodel)#|
#SerializeJSON(ImageInfo(gray).colormodel)#|
#SerializeJSON(ImageInfo(def).colormodel)#|
#SerializeJSON(ImageInfo(def2).colormodel)#
</cfoutput>
