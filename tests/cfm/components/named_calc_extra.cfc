component {
  function greet(who="world") {
    return arguments.who & "|" & structKeyExists(arguments, "zzz");
  }
  function noparam() {
    return structKeyExists(arguments, "a") & "|" & structKeyExists(arguments, "b");
  }
}
