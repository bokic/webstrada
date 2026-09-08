# webstrada 🌐

[![Language: C++23](https://img.shields.io/badge/Language-C%2B%2B23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Compiler: LLVM/Clang](https://img.shields.io/badge/JIT-LLVM-orange.svg)](https://llvm.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

A high-performance, lightweight **CFML Application Engine** written in C++. 

**WebStrada** is a high-performance, **CFML (ColdFusion Markup Language)** application server written in modern C++23. Powered by an LLVM-based JIT compiler and an optimized native runtime, WebStrada delivers Adobe ColdFusion compatibility with near-instant execution speed, ultra-low memory overhead, and native deployment capabilities without Java/JVM dependencies.

---

## ✨ Features

- **⚡ LLVM JIT Compilation:** Directly compiles CFML tags, `<cfscript>`, expressions, and ColdFusion Components (CFCs) into optimized native machine code.
- **🎯 Adobe ColdFusion Compatibility:** Rigorous behavioral alignment with Adobe ColdFusion 2025, verified by differential test suites.
- **☕ Zero JVM Footprint:** Starts in milliseconds, consumes a fraction of the memory required by traditional CF engines, and eliminates JVM warm-up penalties and garbage collection pauses.
- **🧩 Comprehensive Tag & Function Support:**
  - Standard scopes (`Application`, `Session`, `Request`, `Server`, `CGI`, `Variables`, `This`).
  - Database persistence (`<cfquery>`, `<cfqueryparam>`, `<cfstoredproc>`, `<cftransaction>`, Query-of-Queries).
  - Web & HTTP tags (`<cfhttp>`, `<cfheader>`, `<cfcontent>`, `<cflocation>`, `<cfflush>`).
  - Native image processing (`<cfimage>`, Cairo/JPEG), XML, WDDX, ZIP/archive operations, and file/directory I/O.
- **🌐 Deployment Ready:** Runs as a standalone FastCGI application server (`webstrada`) fronted by Nginx/Caddy/Apache, or as a direct command-line runner (`webstrada-cli`).
- **🗄️ Multi-Process Scopes & Caching:** Multi-process session and application caching backed by SQLite WAL mode and shared-memory architectures.
- **🔁 CFScript loop control:** `while` and `do-while` loops, including `break`/`continue`, are compiled and byte-verified against Adobe ColdFusion 2025.
- **⚠️ CFError request context:** `<cferror>` exposes request metadata and location-aware diagnostics in both exception and request handlers.
- **🧵 CFML error stack:** `<cferror>` exposes the captured CFML call stack through `error.stackTrace` and `error.rootCause.stackTrace`.
- **🧪 Test baseline:** The current full unit-suite failure inventory is maintained in `BUGS.md`; log-writing tests require host access to `/var/log/webstrada/` or a writable `WEBSTRADA_LOG_DIR`.

## ⚠️ ColdFusion 2025 feature gaps

WebStrada is not yet a complete implementation of every Adobe ColdFusion 2025 feature. The currently unsupported areas are:

- 2025 language additions that are still pending, including null-coalescing/safe-navigation and lambda/spread syntax, parameter destructuring, several newer assignment/exception constructs, and the newer query-cache options.
- Spreadsheet workbooks and streaming spreadsheets. CSV read/write/process support is available, but the Spreadsheet API is not.
- Server-side charts and the 2025 chart improvements (including `cfchartset` and newer chart types/customization).
- PDF, HTML-to-PDF, document, presentation, and report tags.
- Threading and related interruption/join/termination behavior; WebSocket support.
- Java/.NET object interoperation, ORM/entity/HQL, SOAP/web services, REST lifecycle helpers, SAML/OAuth integrations, gateways, and Exchange/SharePoint integrations.
- The older UI/AJAX/form-control tags and several mail/network/search integrations. `cfmail` is currently only a non-delivering logging stub.
- ColdFusion 2025 Update 8 capabilities: native AI/LLM APIs, MCP client/server support, vector stores/RAG, passkeys/Argon2 security APIs, AI monitoring, native Sets, and CompletableFuture-style async APIs.

---

## 💡 Quick Start

Run `webstrada` by passing a configuration file or defining your routing paths via the command line interface:

```bash
# Download docker images
docker pull bokic78/webstrada:latest

# Create docker container
cd {to root of your CFML application}
docker create --name webstrada -p 80:80 -v .:/webroot bokic78/webstrada:latest

# Start webstrada app server
docker start webstrada

# Stop webstrada app server
docker stop webstrada

# Delete webstrada docker container
docker rm webstrada

# Delete webstrada docker image
docker rmi bokic78/webstrada:latest

```

This will server current directory as CFML application. Then open:

http://localhost/ — the built-in web root (serves the app directory)
http://localhost/webstrada/ — the WebStrada admin panel

---

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

Developed with ❤️ by [Boris Barbulovski (bokic)](https://github.com).
