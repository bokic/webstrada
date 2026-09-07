<cfscript>
// === (strict identity equality) and !== (strict identity inequality) tests

// --- Number vs Number ---
writeOutput("1===1:" & (1===1) & "<br>");          // YES
writeOutput("1===2:" & (1===2) & "<br>");          // NO
writeOutput("1!==1:" & (1!==1) & "<br>");          // NO
writeOutput("1!==2:" & (1!==2) & "<br>");          // YES

// --- Number vs String (different types → not equal) ---
writeOutput("1===""1"":" & (1==="1") & "<br>");    // NO
writeOutput("1!==""1"":" & (1!=="1") & "<br>");    // YES
writeOutput("0===""0"":" & (0==="0") & "<br>");    // NO
writeOutput("0===""false"":" & (0==="false") & "<br>");  // NO

// --- String vs String ---
writeOutput("""a""===""a"":" & ("a"==="a") & "<br>");   // YES
writeOutput("""a""===""b"":" & ("a"==="b") & "<br>");   // NO
writeOutput("""a""===""A"":" & ("a"==="A") & "<br>");   // YES (case-insensitive)
writeOutput("""Hello""===""hello"":" & ("Hello"==="hello") & "<br>");  // YES

// --- Boolean vs Boolean ---
writeOutput("true===true:" & (true===true) & "<br>");    // YES
writeOutput("false===false:" & (false===false) & "<br>");// YES
writeOutput("true===false:" & (true===false) & "<br>");  // NO

// --- Boolean vs Number (different types → not equal) ---
writeOutput("true===1:" & (true===1) & "<br>");          // NO
writeOutput("false===0:" & (false===0) & "<br>");        // NO

// --- Boolean vs String (different types → not equal) ---
writeOutput("true===""true"":" & (true==="true") & "<br>");  // NO
writeOutput("false===""false"":" & (false==="false") & "<br>"); // NO

// --- Float/Integer mixing (both numeric types → compare numerically) ---
writeOutput("1.0===1:" & (1.0===1) & "<br>");    // YES (both numeric)
writeOutput("1.5===1:" & (1.5===1) & "<br>");    // NO

// --- Negation with !== ---
writeOutput("1!==2:" & (1!==2) & "<br>");    // YES
writeOutput("""a""!==1:" & ("a"!==1) & "<br>");  // YES (different types)

// --- In variable context ---
a = 5;
b = 5;
c = "5";
writeOutput("a===b:" & (a===b) & "<br>");   // YES
writeOutput("a===c:" & (a===c) & "<br>");   // NO (Number vs String)
writeOutput("a!==c:" & (a!==c) & "<br>");   // YES

// --- In cfif ---
if (1 === 1) {
    writeOutput("cfif_1===1:YES" & "<br>");
} else {
    writeOutput("cfif_1===1:NO" & "<br>");
}
if (1 === "1") {
    writeOutput("cfif_1===""1"":YES" & "<br>");
} else {
    writeOutput("cfif_1===""1"":NO" & "<br>");
}
</cfscript>
