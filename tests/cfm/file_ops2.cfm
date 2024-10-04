<cfscript>
// Setup: write test file
fw = FileOpen("/tmp/mkf_test_ops2.txt", "write");
FileWriteLine(fw, "line1");
FileWriteLine(fw, "line2");
FileWriteLine(fw, "line3");
FileClose(fw);

// Test 1: FileOpen, FileClose, reassign, and read
WriteOutput("Test1:");
f = FileOpen("/tmp/mkf_test_ops2.txt", "read");
r1 = FileReadLine(f);
FileClose(f);
f = FileOpen("/tmp/mkf_test_ops2.txt", "read");
r1b = FileReadLine(f);
FileClose(f);
WriteOutput(r1 & "," & r1b);

// Test 2: FileSeek + FileSkipBytes are skipped: both abort the CF 2021 test
// server's response, so they cannot be verified against ColdFusion.

// Test 3: FileReadBinary (from path string)
WriteOutput("|Test3:");
b = FileReadBinary("/tmp/mkf_test_ops2.txt");
WriteOutput(IsBinary(b));

// Test 4: FileReadBinary from a file object is skipped: it aborts the CF
// 2021 test server response, so only the path-string form (Test 3) is verified.

// Test 5: FileGetMimeType
WriteOutput("|Test5:");
m = FileGetMimeType("/tmp/mkf_test_ops2.txt");
WriteOutput(m);

// Test 6: ExpandPath
WriteOutput("|Test6:");
exp = ExpandPath(".");
WriteOutput(Len(exp) > 0);

// Test 7: FileSetAccessMode, FileSetAttribute, FileSetLastModified
WriteOutput("|Test7:");
test7f = "/tmp/mkf_test_ops2_attr.txt";
FileWrite(test7f, "test");
FileSetAccessMode(test7f, "644");
FileSetAttribute(test7f, "Normal");
FileSetLastModified(test7f, Now());
WriteOutput("OK");

// Test 8: GetProfileSections, GetProfileString
WriteOutput("|Test8:");
ini = "/tmp/mkf_test_ops2.ini";
f = FileOpen(ini, "write");
FileWriteLine(f, "[section1]");
FileWriteLine(f, "key1=val1");
FileWriteLine(f, "[section2]");
FileWriteLine(f, "key2=val2");
FileClose(f);
sections = GetProfileSections(ini);
val = GetProfileString(ini, "section1", "key1");
FileDelete(ini);
WriteOutput(val);

// Test 9: double close then reopen (the original bug fix test)
WriteOutput("|Test9:");
f9 = FileOpen("/tmp/mkf_test_ops2.txt", "read");
r9a = FileReadLine(f9);
FileClose(f9);
f9b = FileOpen("/tmp/mkf_test_ops2.txt", "read");
r9b = FileReadLine(f9b);
FileClose(f9b);
WriteOutput(r9a & "," & r9b);

// Test 10: FileWriteLine then close then reopen
WriteOutput("|Test10:");
f10 = FileOpen("/tmp/mkf_test_ops2_end.txt", "write");
FileWriteLine(f10, "end");
FileClose(f10);
f10b = FileOpen("/tmp/mkf_test_ops2_end.txt", "read");
r10 = FileReadLine(f10b);
FileClose(f10b);
FileDelete("/tmp/mkf_test_ops2_end.txt");
WriteOutput(r10);

// Test 11: read all 3 lines
WriteOutput("|Test11:");
f11 = FileOpen("/tmp/mkf_test_ops2.txt", "read");
r11a = FileReadLine(f11);
r11b = FileReadLine(f11);
r11c = FileReadLine(f11);
FileClose(f11);
WriteOutput(r11a & "," & r11b & "," & r11c);

// Cleanup
if (FileExists("/tmp/mkf_test_ops2.txt")) FileDelete("/tmp/mkf_test_ops2.txt");
WriteOutput("|DONE");
</cfscript>