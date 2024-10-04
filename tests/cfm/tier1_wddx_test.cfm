<!--- Tier-1: IsWDDX (verified against CF 2025 on the RDS host). --->
<cfoutput>
1:[#IsWDDX('<wddxPacket version="1.0"><header/><data><string>hi</string></data></wddxPacket>')#]
|2:[#IsWDDX('<wddxPacket><header/><data><string>hi</string></data></wddxPacket>')#]
|3:[#IsWDDX('<WDDXPACKET version="1.0"><header/><data><string>hi</string></data></WDDXPACKET>')#]
|4:[#IsWDDX('<wddxPacket version="1.0"><header/><data><number>5</number></data></wddxPacket>')#]
|5:[#IsWDDX('<wddxPacket version="1.0"><header/><data><boolean value="true"/></data></wddxPacket>')#]
|6:[#IsWDDX('<wddxPacket version="1.0"><header/><data><dateTime>1998-6-25T21:15Z</dateTime></data></wddxPacket>')#]
|7:[#IsWDDX('<wddxPacket version="1.0"><header/><data/></wddxPacket>')#]
|8:[#IsWDDX('hello')#]
|9:[#IsWDDX('<wddxPacket version="1.0"><data><string>hi</string></data></wddxPacket>')#]
</cfoutput>
