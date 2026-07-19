# MoDx

[![Build Status](https://img.shields.io/github/actions/workflow/status/MoHong-MHO/MoDx/build.yml?branch=main)](https://github.com/MoHong-MHO/MoDx/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-v1.2--beta-orange)](https://github.com/MoHong-MHO/MoDx/releases)

> A lightweight, multi-threaded command-line downloader built in C.  
> Supports Linux x86_64 and ARM64 with minimal dependencies.

---

## Table of Contents

- [English](#english)
- [中文](#中文)

---

## English

### Overview

MoDx is a lightweight, multi-threaded HTTP downloader written in C. It focuses on simplicity, cross-platform compatibility, and low dependency overhead. Currently supports **Linux x86_64** and **Linux ARM64**.

**Key Features:**

- Multi-threaded download (default 2 threads, configurable up to 16)
- HTTP Range request support
- Resumable download (via `.tmp` file mechanism, planned)
- Real-time progress display (percentage)
- Bilingual interface (English / Chinese, auto-detected)
- Zero extra dependencies beyond standard C library and POSIX sockets
- Lightweight: ~5KB executable, ~6KB shared library

**Supported Platforms:**

| Platform | Status |
|----------|--------|
| Linux x86_64 | ✅ Stable |
| Linux ARM64 | ✅ Stable |
| Android / Alpine | ❌ Not supported |

---

### Installation

#### Option 1: Download Pre-built Binary

Download from [Releases](https://github.com/MoHong-MHO/MoDx/releases) page.

#### Option 2: Build from Source

```bash
git clone https://github.com/MoHong-MHO/MoDx.git
cd MoDx
gcc -o modx main.c modx_lib.c -lpthread -lm -Wall -Wextra
```

For ARM64 cross-compilation:

```bash
sudo apt-get install gcc-aarch64-linux-gnu
aarch64-linux-gnu-gcc -o modx main.c modx_lib.c -lpthread -lm -Wall -Wextra
```

---

Usage

```bash
./modx [options] <URL>
```

Options:

Option Description
-t <threads> Number of download threads (default: 2, max: 16)
-o <filename> Output filename
-h, --help Show help message
-v, --version Show version information

Examples:

```bash
# Basic download
./modx http://example.com/file.zip

# Use 4 threads
./modx -t 4 http://example.com/file.zip

# Specify output filename
./modx -o myfile.zip http://example.com/file.zip

# Combine options
./modx -t 8 -o ubuntu.iso http://releases.ubuntu.com/.../ubuntu.iso
```

---

Project Structure

```
MoDx/
├── main.c          # CLI entry point
├── modx_lib.c      # Download kernel (multi-thread, HTTP, Range)
├── modx_lib.h      # Public API
├── LICENSE         # MIT License
└── README.md       # This file
```

---

License

This project is licensed under the MIT License - see the LICENSE file for details.

---

Acknowledgments

· Built with standard C library and POSIX sockets
· No external dependencies

---

⬆ Back to Top

---

---

中文

概述

MoDx 是一个轻量级的多线程 HTTP 下载器，使用 C 语言编写。它注重简洁性、跨平台兼容性和低依赖。目前支持 Linux x86_64 和 Linux ARM64。

核心特性：

· 多线程下载（默认 2 线程，可配置最多 16 线程）
· HTTP Range 请求支持
· 断点续传支持（通过 .tmp 文件机制，规划中）
· 实时进度显示（百分比）
· 双语界面（英文 / 中文，自动检测）
· 除标准 C 库和 POSIX socket 外无额外依赖
· 轻量级：可执行文件约 5KB，动态库约 6KB

支持平台：

平台 状态
Linux x86_64 ✅ 稳定
Linux ARM64 ✅ 稳定
Android / Alpine ❌ 不支持

---

安装

方式一：下载预编译二进制

从 Releases 页面下载。

方式二：从源码编译

```bash
git clone https://github.com/MoHong-MHO/MoDx.git
cd MoDx
gcc -o modx main.c modx_lib.c -lpthread -lm -Wall -Wextra
```

ARM64 交叉编译：

```bash
sudo apt-get install gcc-aarch64-linux-gnu
aarch64-linux-gnu-gcc -o modx main.c modx_lib.c -lpthread -lm -Wall -Wextra
```

---

使用方法

```bash
./modx [选项] <URL>
```

选项：

选项 说明
-t <线程数> 下载线程数（默认：2，最大：16）
-o <文件名> 输出文件名
-h, --help 显示帮助信息
-v, --version 显示版本信息

示例：

```bash
# 基本下载
./modx http://example.com/file.zip

# 使用 4 线程
./modx -t 4 http://example.com/file.zip

# 指定输出文件名
./modx -o myfile.zip http://example.com/file.zip

# 组合使用
./modx -t 8 -o ubuntu.iso http://releases.ubuntu.com/.../ubuntu.iso
```

---

项目结构

```
MoDx/
├── main.c          # 命令行入口
├── modx_lib.c      # 下载内核（多线程、HTTP、Range）
├── modx_lib.h      # 公开 API
├── LICENSE         # MIT 许可证
└── README.md       # 本文件
```

---

许可证

本项目采用 MIT 许可证 - 详见 LICENSE 文件。

---

致谢

· 使用标准 C 库和 POSIX socket 构建
· 无外部依赖

---

⬆ 回到顶部