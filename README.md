MoDx

[![Build Status](https://img.shields.io/github/actions/workflow/status/MoHong-MHO/MoDx/build.yml?branch=main)](https://github.com/MoHong-MHO/MoDx/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-v1.7-blue)](https://github.com/MoHong-MHO/MoDx/releases)

> A lightweight, multi-threaded HTTP/HTTPS downloader built in C.  
> Supports Linux x86_64 and ARM64 with minimal dependencies.

{1}{1} Table of Contents

- [{1} 1. English](#1-english)
  - [Overview](#overview)
  - [Features](#features)
  - [Installation](#installation)
  - [Usage](#usage)
  - [Options](#options)
  - [Examples](#examples)
  - [Resume Support](#resume-support)
  - [HTTPS Support](#https-support)
  - [Project Structure](#project-structure)
  - [License](#license)
- [{1} 2. 中文](#2-中文)
  - [概述](#概述)
  - [特性](#特性)
  - [安装](#安装)
  - [使用方法](#使用方法)
  - [选项](#选项)
  - [示例](#示例)
  - [断点续传](#断点续传)
  - [HTTPS 支持](#https-支持)
  - [项目结构](#项目结构)
  - [许可证](#许可证)

{1} 1. English

{1}{1} Overview

MoDx is a lightweight, multi-threaded HTTP/HTTPS downloader written in C. It focuses on simplicity, cross-platform compatibility, and low dependency overhead. Currently supports Linux x86_64 and Linux ARM64.

{1}{1} Features

- Multi-threaded download (configurable threads, default 2, max 16)
- Multi-thread resume support (each thread tracks its own progress)
- HTTP and HTTPS support (via mbedTLS, static-linked into libmodx.so)
- HTTP Range request support
- Progress bar display (percentage, speed, ETA)
- Display remote server IP address
- Show HTTP response headers (-H)
- Verbose mode for debugging (-V)
- Auto-resume with user confirmation (-y to skip confirmation)
- Rate limiting (-r)
- Quiet mode (-q)
- Batch download from URL list file (-i)
- Download queue management
- HTTP/HTTPS proxy support (-x)
- Custom DNS server (--dns)
- TLS certificate validation (--ca-cert)
- Mirror download with fallback URLs (-m)
- Checksum verification (MD5/SHA256, enabled by default)
- Custom User-Agent (-u)
- Bilingual interface (English / Chinese, auto-detected by LANG)
- Zero runtime dependencies (everything is linked into libmodx.so)

{1}{1} Installation

{1}{1}{1} Option 1: Download Pre-built Binary

Download from [Releases](https://github.com/MoHong-MHO/MoDx/releases) page.  
The package contains modx (executable) and libmodx.so (shared library).

{1}{1}{1} Option 2: Build from Source

git clone https://github.com/MoHong-MHO/MoDx.git
cd MoDx
gcc -o modx main.c \
    lib/core/*.c lib/download/*.c lib/net/*.c lib/checksum/*.c \
    main/cli/*.c main/ui/*.c main/batch/*.c \
    -lpthread -lm -lmbedtls -lmbedx509 -lmbedcrypto -Wall -Wextra

{1}{1} Usage

./modx [options] <URL>
./modx [options] -i <batch-file>

{1}{1} Options

| Option | Description |
|--------|-------------|
| -t <threads> | Number of download threads (default: 2, max: 16) |
| -o <filename> | Output filename |
| -d <directory> | Output directory (default: current directory) |
| -u <UA> | Custom User-Agent string (default: MoDx/1.7) |
| -R <retries> | Max retries on failure (default: 3) |
| -r <rate> | Rate limit, e.g. 1M (units: B/K/M/G) |
| -q | Quiet mode, no progress output |
| -i <file> | Batch download from URL list file |
| -H | Show HTTP response headers |
| -V | Verbose mode, show debug information |
| -y | Auto-resume without asking |
| -x <proxy> | Use HTTP proxy, e.g. http://proxy:8080 |
| -m <URL...> | Mirror download with fallback URLs |
| --dns <IP> | Custom DNS server |
| --ca-cert <file> | CA certificate file for TLS validation |
| -h, --help | Show help message |
| -v, --version | Show version information |

{1}{1} Examples

Basic HTTP download
./modx http://example.com/file.zip

HTTPS download
./modx https://example.com/file.zip

Use 4 threads and specify output name
./modx -t 4 -o myfile.zip https://example.com/file.zip

Specify output directory and rate limit
./modx -d ./downloads -r 1M https://example.com/file.zip

Show response headers and verbose mode
./modx -H -V https://example.com/file.zip

Auto-resume without asking
./modx -y https://example.com/file.zip

Use proxy
./modx -x http://proxy:8080 https://example.com/file.zip

Mirror download with fallback URLs
./modx -m https://mirror1.com/file.zip https://mirror2.com/file.zip https://example.com/file.zip

Batch download from URL list
./modx -i urls.txt -d ./downloads -t 2

Quiet mode
./modx -q https://example.com/file.zip

{1}{1} Resume Support

MoDx supports multi-thread resume. If a download is interrupted (e.g., Ctrl+C), simply run the same command again to continue from where it left off.

By default, MoDx will ask for confirmation before resuming. Use -y to skip the confirmation and auto-resume.

Progress files:

| File | Description |
|------|-------------|
| filename.progress | Total downloaded bytes |
| filename.progress.0 | Thread 0 progress |
| filename.progress.1 | Thread 1 progress |
| filename.tmp.0 | Thread 0 temporary data |
| filename.tmp.1 | Thread 1 temporary data |

All progress and temporary files are automatically removed upon successful completion.

{1}{1} HTTPS Support

MoDx supports HTTPS via [mbedTLS](https://github.com/Mbed-TLS/mbedtls). The TLS library is statically linked into libmodx.so, so no extra runtime dependencies are required.

By default, certificate verification is disabled for compatibility. To enable verification, use --ca-cert to specify a CA certificate file.

{1}{1} Checksum Verification

MoDx automatically computes and displays MD5 and SHA256 checksums after each successful download. No additional options are required.

{1}{1} Project Structure

MoDx/
├── main.c                      # Entry point
├── lib/
│   ├── core/                   # Core network layer (HTTP, TLS, Socket, DNS)
│   ├── download/               # Download engine (threads, resume, merge, rate)
│   ├── net/                    # Network extensions (proxy, mirror)
│   └── checksum/               # Checksum verification (MD5, SHA256)
├── main/
│   ├── cli/                    # Command line interface (args, help)
│   ├── ui/                     # User interface (progress, output)
│   └── batch/                  # Batch and queue management
├── LICENSE
└── README.md

{1}{1} License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

{1} 2. 中文

{1}{1} 概述

MoDx 是一个轻量级的多线程 HTTP/HTTPS 下载器，使用 C 语言编写。它注重简洁性、跨平台兼容性和低依赖。目前支持 Linux x86_64 和 Linux ARM64。

{1}{1} 特性

- 多线程下载（可配置线程数，默认 2，最大 16）
- 多线程断点续传（每个线程独立记录进度）
- HTTP 和 HTTPS 支持（通过 mbedTLS，静态链接进 libmodx.so）
- HTTP Range 请求支持
- 进度条显示（百分比、速度、剩余时间）
- 显示远程服务器 IP 地址
- 显示 HTTP 响应头（-H）
- 详细调试模式（-V）
- 自动续传（-y 跳过确认）
- 限速下载（-r）
- 静默模式（-q）
- 批量下载（-i）
- 下载队列管理
- HTTP/HTTPS 代理支持（-x）
- 自定义 DNS 服务器（--dns）
- TLS 证书验证（--ca-cert）
- 镜像下载（-m）
- 校验和验证（MD5/SHA256，默认开启）
- 自定义 User-Agent（-u）
- 双语界面（英文 / 中文，通过 LANG 环境变量自动检测）
- 零运行时依赖

{1}{1} 安装

{1}{1}{1} 方式一：下载预编译二进制

从 [Releases](https://github.com/MoHong-MHO/MoDx/releases) 页面下载。  
压缩包包含 modx（可执行文件）和 libmodx.so（动态库）。

{1}{1}{1} 方式二：从源码编译

git clone https://github.com/MoHong-MHO/MoDx.git
cd MoDx
gcc -o modx main.c \
    lib/core/*.c lib/download/*.c lib/net/*.c lib/checksum/*.c \
    main/cli/*.c main/ui/*.c main/batch/*.c \
    -lpthread -lm -lmbedtls -lmbedx509 -lmbedcrypto -Wall -Wextra

{1}{1} 使用方法

./modx [选项] <URL>
./modx [选项] -i <批量文件>

{1}{1} 选项

| 选项 | 说明 |
|------|------|
| -t <线程数> | 下载线程数（默认：2，最大：16） |
| -o <文件名> | 输出文件名 |
| -d <目录> | 输出目录（默认：当前目录） |
| -u <UA> | 自定义 User-Agent（默认：MoDx/1.7） |
| -R <次数> | 失败重试次数（默认：3） |
| -r <速率> | 限速，如 1M（单位：B/K/M/G） |
| -q | 静默模式，不输出进度信息 |
| -i <文件> | 从文件读取 URL 列表批量下载 |
| -H | 显示 HTTP 响应头 |
| -V | 详细模式，显示调试信息 |
| -y | 自动续传，不询问 |
| -x <代理> | 使用 HTTP 代理，如 http://proxy:8080 |
| -m <URL...> | 镜像下载，提供备用 URL |
| --dns <IP> | 自定义 DNS 服务器 |
| --ca-cert <文件> | 指定 CA 证书文件用于 TLS 验证 |
| -h, --help | 显示帮助信息 |
| -v, --version | 显示版本信息 |

{1}{1} 示例

基本 HTTP 下载
./modx http://example.com/file.zip

HTTPS 下载
./modx https://example.com/file.zip

使用 4 线程并指定输出名
./modx -t 4 -o myfile.zip https://example.com/file.zip

指定输出目录和限速
./modx -d ./downloads -r 1M https://example.com/file.zip

显示响应头和详细模式
./modx -H -V https://example.com/file.zip

自动续传不询问
./modx -y https://example.com/file.zip

使用代理
./modx -x http://proxy:8080 https://example.com/file.zip

镜像下载
./modx -m https://mirror1.com/file.zip https://mirror2.com/file.zip https://example.com/file.zip

批量下载
./modx -i urls.txt -d ./downloads -t 2

静默模式
./modx -q https://example.com/file.zip

{1}{1} 断点续传

MoDx 支持多线程断点续传。如果下载被中断（例如按 Ctrl+C），只需再次运行相同命令即可从中断处继续下载。

默认情况下，MoDx 会在续传前询问确认。使用 -y 可跳过确认直接续传。

进度文件说明：

| 文件 | 说明 |
|------|------|
| filename.progress | 总已下载字节数 |
| filename.progress.0 | 线程 0 的进度 |
| filename.progress.1 | 线程 1 的进度 |
| filename.tmp.0 | 线程 0 的临时数据 |
| filename.tmp.1 | 线程 1 的临时数据 |

下载完成后，所有进度文件和临时文件会自动删除。

{1}{1} HTTPS 支持

MoDx 通过 [mbedTLS](https://github.com/Mbed-TLS/mbedtls) 支持 HTTPS。TLS 库被静态链接到 libmodx.so 中，因此不需要额外安装运行时依赖。

默认情况下，证书验证处于禁用状态以保证兼容性。如需启用验证，请使用 --ca-cert 指定 CA 证书文件。

{1}{1} 校验和验证

MoDx 在每次下载完成后自动计算并显示 MD5 和 SHA256 校验和，无需额外选项。

{1}{1} 项目结构

MoDx/
├── main.c                      # 入口
├── lib/
│   ├── core/                   # 核心网络层（HTTP、TLS、Socket、DNS）
│   ├── download/               # 下载引擎（线程、续传、合并、限速）
│   ├── net/                    # 网络扩展（代理、镜像）
│   └── checksum/               # 校验和验证（MD5、SHA256）
├── main/
│   ├── cli/                    # 命令行接口（参数、帮助）
│   ├── ui/                     # 用户界面（进度、输出）
│   └── batch/                  # 批量和队列管理
├── LICENSE
└── README.md

{1}{1} 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。