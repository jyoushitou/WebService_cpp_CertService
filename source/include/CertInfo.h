// CertInfo.h
// 证书信息结构体，支持 JSON 序列化

#ifndef CERTINFO_H
#define CERTINFO_H

#include <string>       // 字符串
#include <vector>       // 动态数组
#include <sstream>      // 字符串流

namespace Cert {

    struct CertInfo {
        std::string commonName;                    // 通用名称 CN
        std::string org;                           // 组织 O
        std::string country;                       // 国家 C
        std::vector<std::string> sanList;          // Subject Alternative Names
        std::string notBefore;                     // 有效期开始
        std::string notAfter;                      // 有效期结束
        std::string serialNumber;                  // 序列号
        std::string fingerprint;                   // SHA256 指纹
        std::string issuer;                        // 颁发者
        bool isCA = false;                         // 是否为 CA 证书
        bool isExpired = false;                    // 是否已过期
        int daysRemaining = 0;                     // 剩余天数

        std::string toJson() const;                // 序列化为 JSON
    };

} // namespace Cert

#endif // !CERTINFO_H