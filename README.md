# webstrada 🌐

[![Language: C](https://shields.io)](https://wikipedia.org)
[![License: LGPL v3](https://shields.io)](https://gnu.org)
[![Build Status](https://shields.io)]()

A high-performance, lightweight, and modular **Web Routing & Network Proxy Engine** written in pure C. 

`webstrada` is built for microservices, edge computing, and custom embedded systems that require microsecond-level routing precision without the overhead of heavy web enterprise frameworks. It acts as a super-fast traffic orchestrator, handling HTTP requests, path routing, and upstream proxying with a minimal memory footprint.

---

## ✨ Features

- **⚡ Zero-Copy Routing:** Highly optimized path matching and request parsing using standard C memory mechanics.
- **🛡️ Lightweight Proxying:** Forward HTTP/HTTPS traffic efficiently to backend upstreams or microservices.
- **🧩 Modular Architecture:** Easily extendable with custom plugins or handlers for request modification and logging.
- **💻 Low Resource Footprint:** Ideal for containerized cloud environments (Docker) and resource-constrained edge hardware.

---

## 🛠️ Building & Installation

### Prerequisites

To compile `webstrada`, you will need a modern C compiler (`gcc` or `clang`), `cmake`, and standard network socket development headers.

*   **Ubuntu / Debian:**
    ```bash
    sudo apt-get install build-essential cmake
    ```
*   **Arch Linux:**
    ```bash
    sudo pacman -S base-devel cmake
    ```

### Build Steps

1. **Clone the repository:**
   ```bash
   git clone https://github.com
   cd webstrada
   ```

2. **Compile the binary:**
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make -j\$(nproc)
   ```

3. **Install the service:**
   ```bash
   sudo make install
   ```

---

## 💡 Quick Start

Run `webstrada` by passing a configuration file or defining your routing paths via the command line interface:

```bash
webstrada --config ./config.json --port 8080
```

### Direct Proxy Mode
```bash
webstrada --listen 0.0.0.0:80 --upstream 127.0.0.1:3000
```

---

## 🤝 Contributing

We welcome structural performance enhancements, bug fixes, and feature requests to make `webstrada` even faster!

1. **Fork** the project.
2. **Create** your feature branch (`git checkout -b feature/AmazingFeature`).
3. **Commit** your changes (`git commit -m 'Add some AmazingFeature'`).
4. **Push** to the branch (`git push origin feature/AmazingFeature`).
5. **Open a Pull Request**.

---

## 📄 License

This project is licensed under the **GNU Lesser General Public License v3.0 (LGPL-3.0)** - see the [LICENSE](LICENSE) file for details.

Developed with ❤️ by [Boris Barbulovski (bokic)](https://github.com).
