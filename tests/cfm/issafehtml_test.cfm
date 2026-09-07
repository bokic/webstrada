<cfscript>
// 1. Safe HTML with default policy
writeOutput("1:[plain:" & isSafeHTML("Plain text without tags") & "]");
writeOutput("2:[p:" & isSafeHTML("<p>Hello world</p>") & "]");
writeOutput("3:[bold italic:" & isSafeHTML("<div><b>Bold</b> and <i>italic</i> and <em>emphasis</em> and <strong>strong</strong></div>") & "]");
writeOutput("4:[blockquote:" & isSafeHTML("<blockquote>A quote</blockquote>") & "]");
writeOutput("5:[lists:" & isSafeHTML("<ul><li>Item 1</li><li>Item 2</li></ul><ol><li>Ordered</li></ol>") & "]");
writeOutput("6:[safe link:" & isSafeHTML("<a href=""https://example.com/path?a=1&b=2"">Link</a>") & "]");
writeOutput("7:[relative link:" & isSafeHTML("<a href=""/local/path"">Relative</a>") & "]");
writeOutput("8:[empty:" & isSafeHTML("") & "]");

// 2. Unsafe HTML with default policy
writeOutput("9:[script:" & isSafeHTML("<script>alert(1)</script>") & "]");
writeOutput("10:[img:" & isSafeHTML("<img src=""image.png"" />") & "]");
writeOutput("11:[iframe:" & isSafeHTML("<iframe src=""http://example.com""></iframe>") & "]");
writeOutput("12:[style tag:" & isSafeHTML("<style>body { color: red; }</style>") & "]");
writeOutput("13:[onclick:" & isSafeHTML("<p onclick=""evil()"">Click me</p>") & "]");
writeOutput("14:[onload:" & isSafeHTML("<body onload=""evil()"">hi</body>") & "]");
writeOutput("15:[javascript link:" & isSafeHTML("<a href=""javascript:alert(1)"">bad link</a>") & "]");
writeOutput("16:[data link:" & isSafeHTML("<a href=""data:text/html,<script>alert(1)</script>"">bad</a>") & "]");
writeOutput("17:[unsupported attr class:" & isSafeHTML("<p class=""test"">text</p>") & "]");
writeOutput("18:[unsupported attr id:" & isSafeHTML("<div id=""myid"">text</div>") & "]");
writeOutput("19:[unsupported attr style:" & isSafeHTML("<p style=""color:red;"">text</p>") & "]");

// 3. String member method
s = "<b>member test</b>";
writeOutput("20:[member:" & s.isSafeHTML() & "]");
sBad = "<script>alert(1)</script>";
writeOutput("21:[member bad:" & sBad.isSafeHTML() & "]");

// 4. Complex object error -> Expression
try {
    isSafeHTML(structNew());
    writeOutput("22:[FAIL: no exception]");
} catch (Expression e) {
    writeOutput("22:[OK:" & e.message & "]");
}

// 5. Non-existent custom policy file -> Application
try {
    isSafeHTML("<p>test</p>", "nonexistent_policy_xyz_12345.xml");
    writeOutput("23:[FAIL: no exception]");
} catch (Application e) {
    writeOutput("23:[OK:" & e.message & "]");
}
</cfscript>
