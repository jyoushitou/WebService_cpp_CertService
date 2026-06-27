// CertConfig.h
// 证书服务配置参数

#ifndef CERTCONFIG_H
#define CERTCONFIG_H

#include <string>       // 字符串
#include <vector>       // 动态数组

namespace Cert {

    struct CertConfig {
        // 服务配置
        int port = 60908;                    // HTTP 服务端口
        std::string domain = "localhost";    // 域名
        int backendPort = 60906;             // 后端 RPC 端口
        std::string certsDir = "./certs";    // 证书输出目录
        std::string nginxDir = "./nginx";    // Nginx 配置输出目录

        // 证书参数
        int days = 3650;                     // 证书有效期（天）
        int keySize = 2048;                  // RSA 密钥长度
        std::string commonName = "localhost"; // 通用名称 CN
        std::string org = "Internal Dev";     // 组织 O
        std::string country = "CN";           // 国家 C
        std::vector<std::string> sanList = {"localhost", "127.0.0.1", "::1"}; // SAN 列表

        // Nginx 配置参数
        int httpPort = 80;                   // HTTP 监听端口
        int httpsPort = 443;                 // HTTPS 监听端口
        bool httpsOnly = false;              // 仅 HTTPS 模式
        std::string staticDir = "/usr/share/nginx/html"; // 前端静态目录
    };

} // namespace Cert

#endif // !CERTCONFIG_H