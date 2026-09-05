# webstrada 🌐

[![Language: C++23](https://img.shields.io/badge/Language-C%2B%2B23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Compiler: LLVM/Clang](https://img.shields.io/badge/JIT-LLVM-orange.svg)](https://llvm.org/)
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL_v3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.html)

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

This project is licensed under the **GNU Lesser General Public License v3.0 (LGPL-3.0)** - see the [LICENSE](LICENSE) file for details.

Developed with ❤️ by [Boris Barbulovski (bokic)](https://github.com).
