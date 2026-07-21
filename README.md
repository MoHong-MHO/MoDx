MoDx

[![Build Status](https://img.shields.io/github/actions/workflow/status/MoHong-MHO/MoDx/build.yml?branch=main)](https://github.com/MoHong-MHO/MoDx/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-v1.6-blue)](https://github.com/MoHong-MHO/MoDx/releases)

> A lightweight, multi-threaded command-line downloader built in C.  
> Supports Linux x86_64 and ARM64 with minimal dependencies.

## Table of Contents

- [# 1. English](#1-english)
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
- [# 2. 中文](#2-中文)
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

# 1. English

## Overview

MoDx is a lightweight, multi-threaded HTTP/HTTPS downloader written in C. It focuses on simplicity, cross-platform compatibility, and low dependency overhead. Currently supports Linux x86_64 and Linux ARM64.

## Features

- Multi-threaded download (configurable threads, default 2, max 16)
- Multi-thread resume support (each thread tracks its own progress)
- HTTP and HTTPS support (via mbedTLS, static-linked into libmodx.so)
- HTTP Range request support
- Real-time progress display (percentage, speed, ETA)
- Display remote server IP address
- Custom User-Agent (-u)
- Rate limiting (-r)
- Quiet mode (-q)
- Batch download from URL list file (-i)
- Command line options (-t, -o, -d, -u, -R, -r, -q, -i, -h, -v)
- Bilingual interface (English / Chinese, auto-detected by LANG)
- Zero runtime dependencies (everything is linked into libmodx.so)
- Lightweight: ~5KB executable, ~100KB shared library (with TLS)

## Installation

### Option 1: Download Pre-built Binary

Download from [Releases](https://github.com/MoHong-MHO/MoDx/releases) page.  
The package contains modx (executable) and libmodx.so (shared library).

### Option 2: Build from Source

git clone https://github.com/MoHong-MHO/MoDx.git
cd MoDx
gcc -o modx main.c -L. -lmodx -Wl,-rpath,.

You also need to build libmodx.so first. The CI script shows how to link mbedTLS statically.

## Usage

./modx [options] <URL>
./modx [options] -i <batch-file>

## Options

| Option | Description |
|--------|-------------|
| -t <threads> | Number of download threads (default: 2, max: 16) |
| -o <filename> | Output filename |
| -d <directory> | Output directory (default: current directory) |
| -u <UA> | Custom User-Agent string (default: MoDx/1.6) |
| -R <retries> | Max retries on failure (default: 3) |
| -r <rate> | Rate limit, e.g. 1M (units: B/K/M/G) |
| -q | Quiet mode, no progress output |
| -i <file> | Batch download from URL list file |
| -h, --help | Show help message |
| -v, --version | Show version information |

## Examples

Basic HTTP download
./modx http://example.com/file.zip

HTTPS download
./modx https://example.com/file.zip

Use 4 threads and specify output name
./modx -t 4 -o myfile.zip https://example.com/file.zip

Specify output directory and rate limit
./modx -d ./downloads -r 1M https://example.com/file.zip

Quiet mode
./modx -q https://example.com/file.zip

Batch download from URL list
./modx -i urls.txt -d ./downloads -t 2

## Resume Support

MoDx supports multi-thread resume. If a download is interrupted (e.g., Ctrl+C), simply run the same command again to continue from where it left off.

Progress files:

| File | Description |
|------|-------------|
| filename.progress | Total downloaded bytes |
| filename.progress.0 | Thread 0 progress |
| filename.progress.1 | Thread 1 progress |
| filename.tmp.0 | Thread 0 temporary data |
| filename.tmp.1 | Thread 1 temporary data |

All progress and temporary files are automatically removed upon successful completion.

## HTTPS Support

MoDx supports HTTPS via [mbedTLS](https://github.com/Mbed-TLS/mbedtls). The TLS library is statically linked into libmodx.so, so no extra runtime dependencies are required.  
The build process compiles mbedTLS with -fPIC to make it linkable into a shared library, then links it into libmodx.so.

## Project Structure

MoDx/
├── main.c          # CLI entry point
├── modx_lib.c      # Download kernel (multi-thread, HTTP/HTTPS, Range, Resume)
├── modx_lib.h      # Public API
├── LICENSE         # MIT License
└── README.md       # This file

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

# 2. 中文

## 概述

MoDx 是一个轻量级的多线程 HTTP/HTTPS 下载器，使用 C 语言编写。它注重简洁性、跨平台兼容性和低依赖。目前支持 Linux x86_64 和 Linux ARM64。

## 特性

- 多线程下载（可配置线程数，默认 2，最大 16）
- 多线程断点续传（每个线程独立记录进度）
- HTTP 和 HTTPS 支持（通过 mbedTLS，静态链接进 libmodx.so）
- HTTP Range 请求支持
- 实时进度显示（百分比、速度、剩余时间）
- 显示远程服务器 IP 地址
- 自定义 User-Agent（-u）
- 限速下载（-r）
- 静默模式（-q）
- 批量下载（-i）
- 命令行选项（-t、-o、-d、-u、-R、-r、-q、-i、-h、-v）
- 双语界面（英文 / 中文，通过 LANG 环境变量自动检测）
- 零运行时依赖（所有 TLS 代码已静态链接）
- 轻量级：可执行文件约 5KB，动态库约 100KB（含 TLS）

## 安装

### 方式一：下载预编译二进制

从 [Releases](https://github.com/MoHong-MHO/MoDx/releases) 页面下载。  
压缩包包含 modx（可执行文件）和 libmodx.so（动态库）。

### 方式二：从源码编译

git clone https://github.com/MoHong-MHO/MoDx.git
cd MoDx
gcc -o modx main.c -L. -lmodx -Wl,-rpath,.

你需要先编译 libmodx.so。CI 脚本展示了如何静态链接 mbedTLS。

## 使用方法

./modx [选项] <URL>
./modx [选项] -i <批量文件>

## 选项

| 选项 | 说明 |
|------|------|
| -t <线程数> | 下载线程数（默认：2，最大：16） |
| -o <文件名> | 输出文件名 |
| -d <目录> | 输出目录（默认：当前目录） |
| -u <UA> | 自定义 User-Agent（默认：MoDx/1.6） |
| -R <次数> | 失败重试次数（默认：3） |
| -r <速率> | 限速，如 1M（单位：B/K/M/G） |
| -q | 静默模式，不输出进度信息 |
| -i <文件> | 从文件读取 URL 列表批量下载 |
| -h, --help | 显示帮助信息 |
| -v, --version | 显示版本信息 |

## 示例

基本 HTTP 下载
./modx http://example.com/file.zip

HTTPS 下载
./modx https://example.com/file.zip

使用 4 线程并指定输出名
./modx -t 4 -o myfile.zip https://example.com/file.zip

指定输出目录和限速
./modx -d ./downloads -r 1M https://example.com/file.zip

静默模式
./modx -q https://example.com/file.zip

从 URL 列表批量下载
./modx -i urls.txt -d ./downloads -t 2

## 断点续传

MoDx 支持多线程断点续传。如果下载被中断（例如按 Ctrl+C），只需再次运行相同命令即可从中断处继续下载。

进度文件说明：

| 文件 | 说明 |
|------|------|
| filename.progress | 总已下载字节数 |
| filename.progress.0 | 线程 0 的进度 |
| filename.progress.1 | 线程 1 的进度 |
| filename.tmp.0 | 线程 0 的临时数据 |
| filename.tmp.1 | 线程 1 的临时数据 |

下载完成后，所有进度文件和临时文件会自动删除。

## HTTPS 支持

MoDx 通过 [mbedTLS](https://github.com/Mbed-TLS/mbedtls) 支持 HTTPS。TLS 库被静态链接到 libmodx.so 中，因此不需要额外安装运行时依赖。  
构建过程中，mbedTLS 会使用 -fPIC 编译，以便能够链接到共享库，然后链接到 libmodx.so。

## 项目结构

MoDx/
├── main.c          # 命令行入口
├── modx_lib.c      # 下载内核（多线程、HTTP/HTTPS、Range、断点续传）
├── modx_lib.h      # 公开 API
├── LICENSE         # MIT 许可证
└── README.md       # 本文件

## 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。