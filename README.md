```markdown
# MoDx

[![Build Status](https://img.shields.io/github/actions/workflow/status/MoHong-MHO/MoDx/build.yml?branch=main)](https://github.com/MoHong-MHO/MoDx/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-v1.4--beta-orange)](https://github.com/MoHong-MHO/MoDx/releases)

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

- Multi-threaded download (configurable threads, default 2, max 16)
- Multi-thread resume support (each thread tracks its own progress)
- HTTP Range request support
- Real-time progress display (percentage, speed, ETA)
- Display remote server IP address
- Custom User-Agent support (`-u`)
- Command line options (`-t`, `-o`, `-u`, `-h`, `-v`)
- Bilingual interface (English / Chinese, auto-detected by LANG)
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
-u <UA> Custom User-Agent string (default: MoDx/1.4)
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

# Custom User-Agent
./modx -u "Mozilla/5.0" http://example.com/file.zip

# Combine options
./modx -t 8 -o ubuntu.iso -u "Mozilla/5.0" http://releases.ubuntu.com/.../ubuntu.iso
```

---

Output Example

```
[Server IP] 93.184.216.34
Starting download...
URL: http://example.com/file.zip
File: file.zip
Threads: 4
User-Agent: MoDx/1.4
[042%] 4.20 MB / 10.00 MB  1.23 MB/s  ETA: 4m42s
```

---

Resume Support

MoDx supports multi-thread resume. If a download is interrupted (e.g., Ctrl+C), simply run the same command again to continue from where it left off.

Progress files:

File Description
filename.progress Total downloaded bytes
filename.progress.0 Thread 0 progress
filename.progress.1 Thread 1 progress
filename.tmp.0 Thread 0 temporary data
filename.tmp.1 Thread 1 temporary data

All progress and temporary files are automatically removed upon successful completion.

---

Project Structure

```
MoDx/
├── main.c          # CLI entry point
├── modx_lib.c      # Download kernel (multi-thread, HTTP, Range, Resume)
├── modx_lib.h      # Public API
├── LICENSE         # MIT License
└── README.md       # This file
```

---

License

This project is licensed under the MIT License - see the LICENSE file for details.

---

⬆ Back to Top

---

---

中文

概述

MoDx 是一个轻量级的多线程 HTTP 下载器，使用 C 语言编写。它注重简洁性、跨平台兼容性和低依赖。目前支持 Linux x86_64 和 Linux ARM64。

核心特性：

· 多线程下载（可配置线程数，默认 2，最大 16）
· 多线程断点续传（每个线程独立记录进度）
· HTTP Range 请求支持
· 实时进度显示（百分比、速度、剩余时间）
· 显示远程服务器 IP 地址
· 自定义 User-Agent（-u）
· 命令行选项（-t、-o、-u、-h、-v）
· 双语界面（英文 / 中文，通过 LANG 环境变量自动检测）
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
-u <UA> 自定义 User-Agent（默认：MoDx/1.4）
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

# 自定义 User-Agent
./modx -u "Mozilla/5.0" http://example.com/file.zip

# 组合使用
./modx -t 8 -o ubuntu.iso -u "Mozilla/5.0" http://releases.ubuntu.com/.../ubuntu.iso
```

---

输出示例

```
[Server IP] 93.184.216.34
开始下载...
URL: http://example.com/file.zip
文件: file.zip
线程数: 4
User-Agent: MoDx/1.4
[042%] 4.20 MB / 10.00 MB  1.23 MB/s  剩余: 4分42秒
```

---

断点续传

MoDx 支持多线程断点续传。如果下载被中断（例如按 Ctrl+C），只需再次运行相同命令即可从中断处继续下载。

进度文件说明：

文件 说明
filename.progress 总已下载字节数
filename.progress.0 线程 0 的进度
filename.progress.1 线程 1 的进度
filename.tmp.0 线程 0 的临时数据
filename.tmp.1 线程 1 的临时数据

下载完成后，所有进度文件和临时文件会自动删除。

---

项目结构

```
MoDx/
├── main.c          # 命令行入口
├── modx_lib.c      # 下载内核（多线程、HTTP、Range、断点续传）
├── modx_lib.h      # 公开 API
├── LICENSE         # MIT 许可证
└── README.md       # 本文件
```

---

许可证

本项目采用 MIT 许可证 - 详见 LICENSE 文件。

---

⬆ 回到顶部

```