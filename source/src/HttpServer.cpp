// HttpServer.cpp
// 轻量 HTTP 服务器实现

#include "../include/HttpServer.h"      // 头文件
#include "../include/CertManager.h"     // 证书管理器
#include "../include/NginxConfigGen.h"  // Nginx 生成器
#include "../include/CertInfo.h"        // 证书信息

#include <iostream>     // 输入输出
#include <ctime>        // 时间

namespace Cert {

    HttpServer::HttpServer(             // 构造函数
        const std::string& host,        // 监听地址
        int port,                       // 监听端口
        CertManager* certManager,       // 证书管理器
        NginxConfigGenerator* nginxGen) // Nginx 生成器
        : m_host(host), m_port(port)    // 初始化地址和端口
        , m_certManager(certManager)    // 初始化证书管理器
        , m_nginxGen(nginxGen)          // 初始化 Nginx 生成器
        , m_ioc(std::make_unique<net::io_context>(1))  // 创建 IO 上下文
    {}

    HttpServer::~HttpServer() {         // 析构函数
        stop();                         // 停止服务器
    }

    bool HttpServer::start() {          // 启动服务器
        try {
            tcp::endpoint endpoint(net::ip::make_address(m_host), m_port);  // 创建端点
            m_acceptor = std::make_unique<tcp::acceptor>(*m_ioc, endpoint);  // 创建接受器
            m_running = true;           // 设置运行标志
            doAccept();                 // 开始接受连接

            m_threads.emplace_back([this]() {  // 创建工作线程
                m_ioc->run();           // 运行 IO 上下文
            });

            return true;                // 启动成功
        } catch (const std::exception& e) {  // 异常处理
            std::cerr << "[CertService] HTTP server start failed: " << e.what() << std::endl;
            return false;               // 启动失败
        }
    }

    void HttpServer::stop() {           // 停止服务器
        if (!m_running.load()) return;  // 已经停止
        m_running = false;              // 清除运行标志
        if (m_acceptor) {               // 接受器存在
            beast::error_code ec;       // 错误码
            m_acceptor->close(ec);      // 关闭接受器
        }
        m_ioc->stop();                  // 停止 IO 上下文
        for (auto& t : m_threads) {     // 遍历线程
            if (t.joinable()) t.join(); // 等待线程结束
        }
        m_threads.clear();              // 清空线程列表
    }

    void HttpServer::doAccept() {       // 接受连接
        if (!m_running) return;         // 未运行
        m_acceptor->async_accept(       // 异步接受
            [this](beast::error_code ec, tcp::socket socket) {  // 回调
                if (!ec) {              // 无错误
                    std::thread(&HttpServer::handleRequest, this, std::move(socket)).detach();  // 处理请求
                }
                doAccept();             // 继续接受
            });
    }

    void HttpServer::handleRequest(tcp::socket socket) {  // 处理请求
        try {
            beast::flat_buffer buffer;  // 缓冲区
            http::request<http::string_body> req;  // HTTP 请求
            http::read(socket, buffer, req);  // 读取请求

            std::string responseBody = handleApiRequest(  // 处理 API 请求
                std::string(req.target()),  // 请求路径
                req.body()                  // 请求体
            );

            http::response<http::string_body> res;  // HTTP 响应
            res.version(req.version());     // 设置 HTTP 版本
            res.result(http::status::ok);   // 设置状态码 200
            res.set(http::field::server, "CertService/1.0");  // 服务器标识
            res.set(http::field::content_type, "application/json");  // 内容类型
            res.set(http::field::access_control_allow_origin, "*");  // CORS
            res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
            res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
            res.body() = responseBody;      // 设置响应体
            res.prepare_payload();          // 准备负载

            http::write(socket, res);       // 写入响应
            socket.shutdown(tcp::socket::shutdown_send);  // 关闭发送
        } catch (const std::exception& e) {  // 异常处理
            std::cerr << "[CertService] Request handler error: " << e.what() << std::endl;
        }
    }

    std::string HttpServer::handleApiRequest(const std::string& path, const std::string& body) {  // API 路由
        if (path == "/api/cert/init") return handleInit();      // 初始化
        if (path == "/api/cert/status") return handleStatus();  // 状态
        if (path == "/api/cert/renew") return handleRenew();    // 续期
        if (path == "/api/cert/pem") return handleCertPem();    // PEM
        if (path == "/api/cert/nginx-conf") return handleNginxConf();  // Nginx 配置
        if (path == "/api/health") return handleHealth();       // 健康检查
        if (path == "/api/info") return handleInfo();           // 信息

        std::ostringstream json;        // JSON 字符串流
        json << "{\"error\": \"not_found\", \"message\": \"Endpoint not found: " << path << "\"}";
        return json.str();              // 返回 404
    }

    std::string HttpServer::handleInit() {  // GET /api/cert/init
        CertInfo info = m_certManager->generateSelfSignedCert();  // 生成证书
        auto nginxFiles = m_nginxGen->saveConfigs(false);  // 生成 Nginx 配置

        std::ostringstream json;        // JSON 字符串流
        json << "{\n";
        json << "  \"status\": \"success\",\n";
        json << "  \"message\": \"Certificate and Nginx config generated\",\n";
        json << "  \"cert_info\": " << info.toJson() << ",\n";  // 证书信息
        json << "  \"nginx_files\": [\n";
        bool first = true;              // 第一个文件标记
        for (const auto& [name, _] : nginxFiles) {  // 遍历文件
            if (!first) json << ",\n";
            json << "    \"" << name << "\"";
            first = false;
        }
        json << "\n  ],\n";
        json << "  \"instructions\": {\n";
        json << "    \"1\": \"Copy certs/ to your nginx ssl directory\",\n";
        json << "    \"2\": \"Copy nginx/nginx.conf to /etc/nginx/nginx.conf\",\n";
        json << "    \"3\": \"Run 'sudo nginx -t && sudo nginx -s reload'\",\n";
        json << "    \"4\": \"Access https://" << m_nginxGen->m_domain << " to verify\"\n";
        json << "  }\n";
        json << "}";
        return json.str();              // 返回 JSON
    }

    std::string HttpServer::handleStatus() {  // GET /api/cert/status
        CertInfo info = m_certManager->loadCertInfo();  // 加载证书信息
        std::ostringstream json;        // JSON 字符串流
        json << "{\n";
        json << "  \"status\": \"success\",\n";
        json << "  \"cert_info\": " << info.toJson() << ",\n";  // 证书信息
        json << "  \"files_exist\": " << (std::filesystem::exists(m_certManager->getCertPath()) ? "true" : "false") << "\n";
        json << "}";
        return json.str();              // 返回 JSON
    }

    std::string HttpServer::handleRenew() {  // POST /api/cert/renew
        CertInfo info = m_certManager->generateSelfSignedCert(  // 强制重新生成
            "localhost", "Internal Dev", "CN",
            {"localhost", "127.0.0.1", "::1"}, 3650, true
        );
        std::ostringstream json;        // JSON 字符串流
        json << "{\n";
        json << "  \"status\": \"success\",\n";
        json << "  \"message\": \"Certificate renewed\",\n";
        json << "  \"cert_info\": " << info.toJson() << "\n";  // 证书信息
        json << "}";
        return json.str();              // 返回 JSON
    }

    std::string HttpServer::handleCertPem() {  // GET /api/cert/pem
        std::string pem = m_certManager->getCertPem();  // 获取证书 PEM
        std::ostringstream json;        // JSON 字符串流
        json << "{\n";
        json << "  \"status\": \"success\",\n";
        json << "  \"cert_pem\": \"";
        for (char c : pem) {            // 转义特殊字符
            if (c == '\n') json << "\\n";
            else if (c == '\"') json << "\\\"";
            else if (c == '\\') json << "\\\\";
            else json << c;
        }
        json << "\"\n";
        json << "}";
        return json.str();              // 返回 JSON
    }

    std::string HttpServer::handleNginxConf() {  // GET /api/cert/nginx-conf
        std::string conf = m_nginxGen->generateNginxConf(false);  // 生成 Nginx 配置
        std::ostringstream json;        // JSON 字符串流
        json << "{\n";
        json << "  \"status\": \"success\",\n";
        json << "  \"nginx_conf\": \"";
        for (char c : conf) {           // 转义特殊字符
            if (c == '\n') json << "\\n";
            else if (c == '\"') json << "\\\"";
            else if (c == '\\') json << "\\\\";
            else json << c;
        }
        json << "\"\n";
        json << "}";
        return json.str();              // 返回 JSON
    }

    std::string HttpServer::handleHealth() {  // GET /api/health
        std::ostringstream json;        // JSON 字符串流
        json << "{\n";
        json << "  \"status\": \"healthy\",\n";
        json << "  \"service\": \"CertService\",\n";
        json << "  \"version\": \"1.0.0\",\n";
        json << "  \"timestamp\": " << std::time(nullptr) << "\n";  // 时间戳
        json << "}";
        return json.str();              // 返回 JSON
    }

    std::string HttpServer::handleInfo() {  // GET /api/info
        std::ostringstream json;        // JSON 字符串流
        json << "{\n";
        json << "  \"service\": \"CertService\",\n";
        json << "  \"version\": \"1.0.0\",\n";
        json << "  \"description\": \"Auto SSL certificate distribution service\",\n";
        json << "  \"endpoints\": {\n";
        json << "    \"GET /api/cert/init\": \"Generate and get certificate + nginx config\",\n";
        json << "    \"GET /api/cert/status\": \"Check certificate status\",\n";
        json << "    \"POST /api/cert/renew\": \"Force renew certificate\",\n";
        json << "    \"GET /api/cert/pem\": \"Get certificate PEM content\",\n";
        json << "    \"GET /api/cert/nginx-conf\": \"Get nginx configuration\",\n";
        json << "    \"GET /api/health\": \"Health check\",\n";
        json << "    \"GET /api/info\": \"Service information\"\n";
        json << "  },\n";
        json << "  \"ports\": {\n";
        json << "    \"cert_service\": " << m_port << ",\n";  // 证书服务端口
        json << "    \"backend_rpc\": " << (m_nginxGen->m_backendPort) << "\n";  // 后端 RPC 端口
        json << "  }\n";
        json << "}";
        return json.str();              // 返回 JSON
    }

} // namespace Cert