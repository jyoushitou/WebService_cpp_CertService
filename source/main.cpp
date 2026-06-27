// main.cpp
// 证书微服务 — 程序入口

#include "include/CertService.h"   // 微服务主类
#include "include/CertManager.h"   // 证书管理器
#include "include/NginxConfigGen.h" // Nginx 生成器
#include "include/CertInfo.h"       // 证书信息
#include "include/CertConfig.h"     // 配置结构体

#include <iostream>     // 输入输出
#include <string>       // 字符串
#include <csignal>      // 信号处理
#include <atomic>       // 原子操作
#include <thread>       // 线程
#include <chrono>       // 时间

#ifdef _WIN32
#include <windows.h>    // Windows API
#endif

Cert::CertService* g_service = nullptr;     // 全局服务实例指针
std::atomic<bool> g_running{true};          // 运行标志

void signalHandler(int signal) {            // 信号处理函数
    std::cout << "[CertService] Received signal " << signal << ", shutting down..." << std::endl;
    g_running = false;                      // 清除运行标志
    if (g_service) {                        // 服务实例存在
        g_service->stop();                  // 停止服务
    }
}

void printHelp(const char* programName) {   // 打印帮助信息
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  (no mode flag)         Start HTTP server (default)" << std::endl;
    std::cout << "  --gen-cert-only        Generate SSL certificate only" << std::endl;
    std::cout << "  --gen-nginx-only       Generate Nginx config only" << std::endl;
    std::cout << "  --setup                Generate certificate + Nginx config" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --port <port>          HTTP server port (default: 60908)" << std::endl;
    std::cout << "  --domain <domain>      Domain name (default: localhost)" << std::endl;
    std::cout << "  --backend-port <port>  Backend RPC port (default: 60906)" << std::endl;
    std::cout << "  --certs-dir <dir>      Certificate output directory (default: ./certs)" << std::endl;
    std::cout << "  --nginx-dir <dir>      Nginx config output directory (default: ./nginx)" << std::endl;
    std::cout << "  --days <days>          Certificate validity (default: 3650)" << std::endl;
    std::cout << "  --key-size <size>      RSA key size 2048|4096 (default: 2048)" << std::endl;
    std::cout << "  --common-name <cn>     Certificate common name (default: localhost)" << std::endl;
    std::cout << "  --org <org>            Organization name (default: Internal Dev)" << std::endl;
    std::cout << "  --country <code>       Country code (default: CN)" << std::endl;
    std::cout << "  --san <names...>       Subject Alternative Names" << std::endl;
    std::cout << "  --http-port <port>     Nginx HTTP listen port (default: 80)" << std::endl;
    std::cout << "  --https-port <port>    Nginx HTTPS listen port (default: 443)" << std::endl;
    std::cout << "  --https-only           HTTPS only mode for nginx" << std::endl;
    std::cout << "  --static-dir <dir>     Frontend static files directory" << std::endl;
    std::cout << "  --help                 Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " --port 60908 --domain myapp.local" << std::endl;
    std::cout << "  " << programName << " --gen-cert-only --certs-dir ./certs" << std::endl;
    std::cout << "  " << programName << " --setup --domain myapp.local --backend-port 8080" << std::endl;
}

int runGenCertOnly(const Cert::CertConfig& config) {  // 命令行：仅生成证书
    Cert::CertManager manager(config.certsDir, config.keySize);  // 创建证书管理器
    Cert::CertInfo info = manager.generateSelfSignedCert(  // 生成证书
        config.commonName, config.org, config.country,
        config.sanList, config.days, true
    );
    std::cout << info.toJson() << std::endl;  // 输出 JSON
    return 0;
}

int runGenNginxOnly(const Cert::CertConfig& config) {  // 命令行：仅生成 Nginx 配置
    Cert::NginxConfigGenerator gen(  // 创建 Nginx 生成器
        config.nginxDir, config.domain,
        config.httpPort, config.httpsPort,
        config.backendPort,
        config.staticDir,
        config.certsDir + "/server.crt",
        config.certsDir + "/server.key"
    );
    auto files = gen.saveConfigs(config.httpsOnly);  // 保存配置
    std::cout << "{\"status\":\"success\",\"files\":[";
    bool first = true;
    for (const auto& [name, _] : files) {
        if (!first) std::cout << ",";
        std::cout << "\"" << name << "\"";
        first = false;
    }
    std::cout << "]}" << std::endl;
    return 0;
}

int runSetup(const Cert::CertConfig& config) {  // 命令行：一键部署
    std::cout << "[CertService] Running setup..." << std::endl;
    
    std::cout << "[1/2] Generating SSL certificate..." << std::endl;
    Cert::CertManager manager(config.certsDir, config.keySize);  // 创建证书管理器
    Cert::CertInfo info = manager.generateSelfSignedCert(  // 生成证书
        config.commonName, config.org, config.country,
        config.sanList, config.days, true
    );
    std::cout << "[OK] Certificate generated: " << info.commonName
              << " (expires in " << info.daysRemaining << " days)" << std::endl;
    
    std::cout << "[2/2] Generating Nginx configuration..." << std::endl;
    Cert::NginxConfigGenerator gen(  // 创建 Nginx 生成器
        config.nginxDir, config.domain,
        config.httpPort, config.httpsPort,
        config.backendPort,
        config.staticDir,
        config.certsDir + "/server.crt",
        config.certsDir + "/server.key"
    );
    auto files = gen.saveConfigs(config.httpsOnly);  // 保存配置
    std::cout << "[OK] Nginx config generated:" << std::endl;
    for (const auto& [name, _] : files) {
        std::cout << "     - " << config.nginxDir << "/" << name << std::endl;
    }
    
    std::cout << "[Done] Setup complete!" << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {  // 主函数
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);    // 设置 UTF-8 编码
#endif

    std::signal(SIGINT, signalHandler);   // 注册 SIGINT 处理
    std::signal(SIGTERM, signalHandler);  // 注册 SIGTERM 处理

    Cert::CertConfig config;        // 默认配置
    config.port = 60908;            // HTTP 服务端口
    config.domain = "localhost";    // 域名
    config.backendPort = 60906;     // 后端 RPC 端口
    config.certsDir = "./certs";    // 证书输出目录
    config.nginxDir = "./nginx";    // Nginx 配置输出目录
    config.days = 3650;             // 证书有效期
    config.keySize = 2048;          // RSA 密钥长度
    config.commonName = "localhost"; // 通用名称
    config.org = "Internal Dev";    // 组织
    config.country = "CN";          // 国家
    config.sanList = {"localhost", "127.0.0.1", "::1"};  // SAN 列表
    config.httpPort = 80;           // HTTP 端口
    config.httpsPort = 443;         // HTTPS 端口
    config.httpsOnly = false;       // 非仅 HTTPS
    config.staticDir = "/usr/share/nginx/html";  // 静态目录
    
    enum class Mode { Service, GenCertOnly, GenNginxOnly, Setup };  // 运行模式
    Mode mode = Mode::Service;      // 默认服务模式

    for (int i = 1; i < argc; ++i) {  // 解析命令行参数
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {  // 帮助
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--gen-cert-only") {  // 仅生成证书
            mode = Mode::GenCertOnly;
        } else if (arg == "--gen-nginx-only") {  // 仅生成 Nginx 配置
            mode = Mode::GenNginxOnly;
        } else if (arg == "--setup") {  // 一键部署
            mode = Mode::Setup;
        } else if (arg == "--port" && i + 1 < argc) {  // 端口
            config.port = std::stoi(argv[++i]);
        } else if (arg == "--domain" && i + 1 < argc) {  // 域名
            config.domain = argv[++i];
        } else if (arg == "--backend-port" && i + 1 < argc) {  // 后端端口
            config.backendPort = std::stoi(argv[++i]);
        } else if (arg == "--certs-dir" && i + 1 < argc) {  // 证书目录
            config.certsDir = argv[++i];
        } else if (arg == "--nginx-dir" && i + 1 < argc) {  // Nginx 目录
            config.nginxDir = argv[++i];
        } else if (arg == "--days" && i + 1 < argc) {  // 有效期
            config.days = std::stoi(argv[++i]);
        } else if (arg == "--key-size" && i + 1 < argc) {  // 密钥长度
            config.keySize = std::stoi(argv[++i]);
        } else if (arg == "--common-name" && i + 1 < argc) {  // 通用名称
            config.commonName = argv[++i];
        } else if (arg == "--org" && i + 1 < argc) {  // 组织
            config.org = argv[++i];
        } else if (arg == "--country" && i + 1 < argc) {  // 国家
            config.country = argv[++i];
        } else if (arg == "--san") {  // SAN 列表
            config.sanList.clear();
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                config.sanList.push_back(argv[++i]);
            }
        } else if (arg == "--http-port" && i + 1 < argc) {  // HTTP 端口
            config.httpPort = std::stoi(argv[++i]);
        } else if (arg == "--https-port" && i + 1 < argc) {  // HTTPS 端口
            config.httpsPort = std::stoi(argv[++i]);
        } else if (arg == "--https-only") {  // 仅 HTTPS
            config.httpsOnly = true;
        } else if (arg == "--static-dir" && i + 1 < argc) {  // 静态目录
            config.staticDir = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printHelp(argv[0]);
            return 1;
        }
    }

    switch (mode) {  // 根据模式执行
        case Mode::GenCertOnly:
            return runGenCertOnly(config);
        case Mode::GenNginxOnly:
            return runGenNginxOnly(config);
        case Mode::Setup:
            return runSetup(config);
        case Mode::Service:
            break;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "  CertService v1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Port:          " << config.port << std::endl;
    std::cout << "  Domain:        " << config.domain << std::endl;
    std::cout << "  Backend Port:  " << config.backendPort << std::endl;
    std::cout << "  Certs Dir:     " << config.certsDir << std::endl;
    std::cout << "  Nginx Dir:     " << config.nginxDir << std::endl;
    std::cout << "========================================" << std::endl;

    Cert::CertService service;      // 创建服务
    g_service = &service;           // 设置全局指针

    if (!service.initialize(config)) {  // 初始化
        std::cerr << "[CertService] Failed to initialize service" << std::endl;
        return 1;
    }

    if (!service.start()) {         // 启动
        std::cerr << "[CertService] Failed to start service" << std::endl;
        return 1;
    }

    std::cout << "[CertService] Service is running. Press Ctrl+C to stop." << std::endl;

    while (g_running) {             // 主循环
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[CertService] Shutdown complete" << std::endl;
    return 0;
}