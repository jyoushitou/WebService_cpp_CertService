// NginxConfigGen.h
// Nginx 配置文件生成器

#ifndef NGINXCONFIGGEN_H
#define NGINXCONFIGGEN_H

#include <string>       // 字符串
#include <map>          // 键值对
#include <sstream>      // 字符串流
#include <fstream>      // 文件流
#include <filesystem>   // 文件系统

namespace Cert {

    class NginxConfigGenerator {
    public:
        NginxConfigGenerator(                       // 构造函数
            const std::string& outputDir,           // 输出目录
            const std::string& domain = "localhost", // 域名
            int httpPort = 80,                      // HTTP 端口
            int httpsPort = 443,                    // HTTPS 端口
            int backendPort = 60906,                // 后端 RPC 端口
            const std::string& staticDir = "/usr/share/nginx/html", // 静态目录
            const std::string& certFile = "/etc/nginx/certs/server.crt", // 证书路径
            const std::string& keyFile = "/etc/nginx/certs/server.key"   // 私钥路径
        );

        std::string generateNginxConf(bool httpsOnly = false);  // 生成主配置
        std::string generateSslConf();                           // 生成 SSL 片段
        std::string generateDockerComposeOverride();             // 生成 Docker 覆盖
        std::map<std::string, std::string> saveConfigs(bool httpsOnly = false); // 保存所有

        std::string m_domain;       // 域名（公开供外部访问）
        int m_backendPort;          // 后端端口（公开供外部访问）

    private:
        std::string m_outputDir;    // 输出目录
        int m_httpPort;             // HTTP 端口
        int m_httpsPort;            // HTTPS 端口
        std::string m_staticDir;    // 静态文件目录
        std::string m_certFile;     // 证书文件路径
        std::string m_keyFile;      // 私钥文件路径
    };

} // namespace Cert

#endif // !NGINXCONFIGGEN_H