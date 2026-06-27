// CertService.h
// 证书微服务主类，协调各模块

#ifndef CERTSERVICE_H
#define CERTSERVICE_H

#include <memory>       // 智能指针
#include <atomic>       // 原子操作
#include <thread>       // 线程

#include "CertConfig.h" // 配置结构体

namespace Cert {

    class CertManager;              // 前向声明
    class NginxConfigGenerator;     // 前向声明
    class HttpServer;               // 前向声明

    class CertService {
    public:
        CertService();              // 构造函数
        ~CertService();             // 析构函数

        bool initialize(const CertConfig& config);  // 初始化
        bool start();                                // 启动服务
        void stop();                                 // 停止服务
        void wait();                                 // 等待结束

        CertManager* getCertManager() { return m_certManager.get(); }       // 获取证书管理器
        NginxConfigGenerator* getNginxGenerator() { return m_nginxGen.get(); } // 获取 Nginx 生成器

    private:
        std::unique_ptr<CertManager> m_certManager;         // 证书管理器
        std::unique_ptr<NginxConfigGenerator> m_nginxGen;   // Nginx 生成器
        std::unique_ptr<HttpServer> m_httpServer;           // HTTP 服务器

        CertConfig m_config;                    // 配置
        std::atomic<bool> m_running{false};     // 运行标志
        std::thread m_certCheckThread;          // 证书检查线程
    };

} // namespace Cert

#endif // !CERTSERVICE_H