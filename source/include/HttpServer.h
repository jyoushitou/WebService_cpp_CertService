// HttpServer.h
// 轻量 HTTP 服务器，提供证书分发 API

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>       // 字符串
#include <memory>       // 智能指针
#include <atomic>       // 原子操作
#include <thread>       // 线程
#include <vector>       // 动态数组

#include <boost/beast/core.hpp>     // Boost.Beast 核心
#include <boost/beast/http.hpp>     // Boost.Beast HTTP
#include <boost/asio/ip/tcp.hpp>    // Boost.Asio TCP
#include <boost/asio/strand.hpp>    // Boost.Asio Strand

namespace beast = boost::beast;     // Beast 命名空间
namespace http = beast::http;       // HTTP 命名空间
namespace net = boost::asio;        // Asio 命名空间
using tcp = net::ip::tcp;           // TCP 类型

namespace Cert {

    class CertManager;              // 前向声明
    class NginxConfigGenerator;     // 前向声明

    class HttpServer {
    public:
        HttpServer(                 // 构造函数
            const std::string& host,            // 监听地址
            int port,                           // 监听端口
            CertManager* certManager,           // 证书管理器指针
            NginxConfigGenerator* nginxGen      // Nginx 生成器指针
        );
        ~HttpServer();                          // 析构函数

        bool start();               // 启动服务器
        void stop();                // 停止服务器
        bool isRunning() const { return m_running.load(); } // 是否运行中

    private:
        void doAccept();            // 接受连接
        void handleRequest(tcp::socket socket); // 处理请求
        std::string handleApiRequest(const std::string& path, const std::string& body); // API 路由

        std::string handleInit();           // GET /api/cert/init
        std::string handleStatus();         // GET /api/cert/status
        std::string handleRenew();          // POST /api/cert/renew
        std::string handleCertPem();        // GET /api/cert/pem
        std::string handleNginxConf();      // GET /api/cert/nginx-conf
        std::string handleHealth();         // GET /api/health
        std::string handleInfo();           // GET /api/info

        std::string m_host;                 // 监听地址
        int m_port;                         // 监听端口
        CertManager* m_certManager;         // 证书管理器
        NginxConfigGenerator* m_nginxGen;   // Nginx 生成器

        std::unique_ptr<net::io_context> m_ioc;     // IO 上下文
        std::unique_ptr<tcp::acceptor> m_acceptor;  // 接受器
        std::atomic<bool> m_running{false};         // 运行标志
        std::vector<std::thread> m_threads;         // 工作线程
    };

} // namespace Cert

#endif // !HTTPSERVER_H