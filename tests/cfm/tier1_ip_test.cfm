<!--- Tier-1: GetLocalHostIP / IsIPv6 / IsLocalHost (verified against CF 2025). --->
<cfoutput>
1:[#GetLocalHostIP()#]|2:[#IsIPv6("::1")#]|3:[#IsIPv6("127.0.0.1")#]|4:[#IsIPv6("fe80::1")#]|5:[#IsIPv6("foo")#]|6:[#IsIPv6("192.168.1.1")#]|7:[#IsIPv6("2001:db8::1")#]|8:[#IsIPv6("::ffff:127.0.0.1")#]|9:[#IsIPv6("0:0:0:0:0:0:0:1")#]|10:[#IsIPv6("::")#]|11:[#IsIPv6("localhost")#]|12:[#IsIPv6("127.0.0.2")#]
13:[#IsLocalHost("127.0.0.1")#]|14:[#IsLocalHost("localhost")#]|15:[#IsLocalHost("::1")#]|16:[#IsLocalHost("192.168.100.10")#]|17:[#IsLocalHost("8.8.8.8")#]|18:[#IsLocalHost("127.0.0.2")#]|19:[#IsLocalHost("LOCALHOST")#]|20:[#IsLocalHost("  localhost  ")#]|21:[#IsLocalHost("0:0:0:0:0:0:0:1")#]|22:[#IsLocalHost("[::1]")#]
</cfoutput>
