// CertService.cpp
// 证书微服务主类实现

#include "../include/CertService.h"    // 头文件
#include "../include/CertManager.h"   // 证书管理器
#include "../include/NginxConfigGen.h" // Nginx 生成器
#include "../include/HttpServer.h"     // HTTP 服务器
#include "../include/CertInfo.h"       // 证书信息

#include <iostream>     // 输入输出
#include <chrono>       // 时间

namespace Cert {

    CertService::CertService() {}       // 构造函数

    CertService::~CertService() {       // 析构函数
        stop();                         // 停止服务
    }

    bool CertService::initialize(const CertConfig& config) {  // 初始化
        m_config = config;              // 保存配置

        m_certManager = std::make_unique<CertManager>(config.certsDir, config.keySize);  // 创建证书管理器

        m_nginxGen = std::make_unique<NginxConfigGenerator>(  // 创建 Nginx 生成器
            config.nginxDir, config.domain,
            config.httpPort, config.httpsPort,
            config.backendPort,
            config.staticDir,
            config.certsDir + "/server.crt",
            config.certsDir + "/server.key"
        );

        m_httpServer = std::make_unique<HttpServer>(  // 创建 HTTP 服务器
            "0.0.0.0", config.port,
            m_certManager.get(),
            m_nginxGen.get()
        );

        return true;                    // 初始化成功
    }

    bool CertService::start() {         // 启动服务
        std::cout << "[CertService] Generating initial certificate..." << std::endl;
        CertInfo info = m_certManager->generateSelfSignedCert();  // 生成初始证书
        std::cout << "[CertService] Certificate generated: " << info.commonName
                  << " (expires in " << info.daysRemaining << " days)" << std::endl;

        std::cout << "[CertService] Generating Nginx configuration..." << std::endl;
        m_nginxGen->saveConfigs(false); // 生成 Nginx 配置
        std::cout << "[CertService] Nginx config saved to: " << m_config.nginxDir << std::endl;

        std::cout << "[CertService] Starting HTTP server on port " << m_config.port << "..." << std::endl;
        if (!m_httpServer->start()) {   // 启动 HTTP 服务器
            std::cerr << "[CertService] Failed to start HTTP server" << std::endl;
            return false;               // 启动失败
        }

        m_running = true;               // 设置运行标志
        std::cout << "[CertService] Service started successfully!" << std::endl;
        std::cout << "[CertService] Frontend can access: http://localhost:" << m_config.port << "/api/cert/init" << std::endl;

        m_certCheckThread = std::thread([this]() {  // 证书检查线程
            while (m_running) {         // 服务运行中
                std::this_thread::sleep_for(std::chrono::hours(24));  // 每天检查一次
                if (m_running) {        // 服务仍在运行
                    CertInfo info = m_certManager->checkAndRenew(30);  // 检查并续期
                    if (info.isExpired) {  // 已过期
                        std::cout << "[CertService] Certificate renewed automatically" << std::endl;
                    }
                }
            }
        });

        return true;                    // 启动成功
    }

    void CertService::stop() {          // 停止服务
        if (!m_running.load()) return;  // 已经停止
        m_running = false;              // 清除运行标志

        if (m_httpServer) {             // HTTP 服务器存在
            m_httpServer->stop();       // 停止 HTTP 服务器
        }

        if (m_certCheckThread.joinable()) {  // 证书检查线程可加入
            m_certCheckThread.join();   // 等待线程结束
        }

        std::cout << "[CertService] Service stopped" << std::endl;
    }

    void CertService::wait() {          // 等待结束
        if (m_certCheckThread.joinable()) {  // 证书检查线程可加入
            m_certCheckThread.join();   // 等待线程结束
        }
    }

} // namespace Cert