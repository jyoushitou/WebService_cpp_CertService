# CertService
# 自签名 SSL 证书微服务

## 概述

CertService 是一个 C++ 微服务，用于自动生成自签名 SSL 证书和 Nginx HTTPS 配置。

## 功能

- 自动生成自签名 SSL 证书（支持 SAN）
- 自动生成 Nginx HTTPS 配置
- HTTP API 接口，供前端调用
- 证书自动续期检查
- 支持命令行模式（供 Python 脚本调用）

## 构建

### 前置条件

- CMake 3.14+
- C++17 编译器
- OpenSSL
- Boost（Beast、Asio、System）

### 构建步骤

```bash
cd CertService
mkdir build && cd build
cmake ..
cmake --build .
```

## 使用

### 服务模式

```bash
./CertService                    # 启动 HTTP 服务（端口 60908）
./CertService --port 60908       # 自定义端口
./CertService --domain myapp.local  # 自定义域名
```

### 命令行模式

```bash
./CertService --gen-cert-only    # 仅生成证书
./CertService --gen-nginx-only   # 仅生成 Nginx 配置
./CertService --setup            # 一键部署
```

### Python 部署脚本

```bash
cd deploy
python setup.py                  # 一键部署
python gen_cert.py               # 仅生成证书
python gen_nginx_conf.py         # 仅生成 Nginx 配置
```

## API 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | /api/cert/init | 生成并获取证书 + Nginx 配置 |
| GET | /api/cert/status | 检查证书状态 |
| POST | /api/cert/renew | 强制续期证书 |
| GET | /api/cert/pem | 获取证书 PEM 内容 |
| GET | /api/cert/nginx-conf | 获取 Nginx 配置 |
| GET | /api/health | 健康检查 |
| GET | /api/info | 服务信息 |

## 端口

- 60908：CertService HTTP API
- 60906：后端 RPC 服务（由 Nginx 代理）

## 目录结构

```
CertService/
├── include/          # 头文件
├── src/              # 源文件
├── deploy/           # Python 部署脚本
├── CMakeLists.txt    # 构建配置
├── main.cpp          # 程序入口
└── README.md         # 本文件
```