// CertManager.h
// 自签名 SSL 证书管理器

#ifndef CERTMANAGER_H
#define CERTMANAGER_H

#include <string>       // 字符串
#include <vector>       // 动态数组
#include <map>          // 键值对
#include <mutex>        // 互斥锁
#include <fstream>      // 文件流
#include <filesystem>   // 文件系统

#include <openssl/ssl.h>    // OpenSSL SSL
#include <openssl/err.h>    // OpenSSL 错误
#include <openssl/pem.h>    // PEM 编码
#include <openssl/x509.h>   // X509 证书
#include <openssl/x509v3.h> // X509 V3 扩展
#include <openssl/rsa.h>    // RSA 密钥
#include <openssl/evp.h>    // EVP 加密
#include <openssl/bn.h>     // 大数运算
#include <openssl/rand.h>   // 随机数

#include "CertInfo.h"      // 证书信息结构体

namespace Cert {

    class CertManager {
    public:
        CertManager(const std::string& certsDir, int keySize = 2048);   // 构造函数
        ~CertManager();                                                  // 析构函数

        CertInfo generateSelfSignedCert(             // 生成自签名证书
            const std::string& commonName = "localhost",
            const std::string& org = "Internal Dev",
            const std::string& country = "CN",
            const std::vector<std::string>& sanList = {"localhost", "127.0.0.1", "::1"},
            int days = 3650,
            bool force = false
        );

        CertInfo loadCertInfo();                     // 加载证书信息
        CertInfo checkAndRenew(                      // 检查并续期
            int renewBeforeDays = 30,
            const std::string& commonName = "localhost",
            const std::string& org = "Internal Dev",
            const std::string& country = "CN",
            const std::vector<std::string>& sanList = {"localhost", "127.0.0.1", "::1"},
            int days = 3650
        );

        std::string getCertPem();                    // 获取证书 PEM
        std::string getKeyPem();                     // 获取私钥 PEM
        std::map<std::string, std::string> getCertFiles(); // 获取所有证书文件

        std::string getKeyPath() const { return m_certsDir + "/server.key"; }   // 私钥路径
        std::string getCertPath() const { return m_certsDir + "/server.crt"; }  // 证书路径
        std::string getPemPath() const { return m_certsDir + "/server.pem"; }   // PEM 路径
        std::string getInfoPath() const { return m_certsDir + "/cert_info.json"; } // 信息路径

    private:
        void ensureDirs();                           // 确保目录存在
        void saveCertInfo(const CertInfo& info);     // 保存证书信息
        std::string sha256Hex(const unsigned char* data, size_t len); // SHA256 转十六进制

        std::string m_certsDir;                      // 证书目录
        int m_keySize;                               // 密钥长度
        std::mutex m_mutex;                          // 互斥锁
    };

} // namespace Cert

#endif // !CERTMANAGER_H