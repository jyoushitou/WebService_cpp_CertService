// CertManager.cpp
// 自签名 SSL 证书管理器实现

#include "../include/CertManager.h" // 头文件

namespace Cert {

    CertManager::CertManager(const std::string& certsDir, int keySize)  // 构造函数
        : m_certsDir(certsDir), m_keySize(keySize)  // 初始化成员
    {
        ensureDirs();   // 确保目录存在
    }

    CertManager::~CertManager() {}  // 析构函数

    void CertManager::ensureDirs() {    // 确保目录存在
        std::filesystem::create_directories(m_certsDir);  // 创建目录
    }

    std::string CertManager::sha256Hex(const unsigned char* data, size_t len) {  // SHA256 转十六进制
        unsigned char hash[32];         // SHA256 哈希值（32 字节）
        SHA256(data, len, hash);        // 计算 SHA256
        std::ostringstream hex;         // 十六进制字符串流
        for (int i = 0; i < 32; ++i) {  // 遍历 32 字节
            hex << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];  // 转十六进制
        }
        return hex.str();               // 返回十六进制字符串
    }

    CertInfo CertManager::generateSelfSignedCert(   // 生成自签名证书
        const std::string& commonName,              // 通用名称
        const std::string& org,                     // 组织
        const std::string& country,                 // 国家
        const std::vector<std::string>& sanList,    // SAN 列表
        int days,                                   // 有效期
        bool force)                                 // 是否强制重新生成
    {
        std::lock_guard<std::mutex> lock(m_mutex);  // 加锁

        if (!force) {                               // 不强制重新生成
            std::ifstream certFile(getCertPath());  // 检查证书文件
            std::ifstream keyFile(getKeyPath());    // 检查私钥文件
            if (certFile.good() && keyFile.good()) { // 文件都存在
                return loadCertInfo();              // 返回已有证书信息
            }
        }

        CertInfo info;                              // 证书信息
        info.commonName = commonName;               // 通用名称
        info.org = org;                             // 组织
        info.country = country;                     // 国家
        info.sanList = sanList;                     // SAN 列表

        EVP_PKEY* pkey = EVP_PKEY_new();            // 创建 EVP 密钥对
        RSA* rsa = RSA_new();                       // 创建 RSA 密钥
        BIGNUM* bn = BN_new();                      // 创建大数
        BN_set_word(bn, RSA_F4);                    // 设置公钥指数 65537
        RSA_generate_key_ex(rsa, m_keySize, bn, nullptr);  // 生成 RSA 密钥对
        EVP_PKEY_assign_RSA(pkey, rsa);             // 将 RSA 赋值给 EVP_PKEY

        X509* cert = X509_new();                    // 创建 X509 证书
        ASN1_INTEGER_set(X509_get_serialNumber(cert), std::time(nullptr));  // 设置序列号

        X509_gmtime_adj(X509_get_notBefore(cert), 0);   // 设置有效期开始
        X509_gmtime_adj(X509_get_notAfter(cert), 60 * 60 * 24 * days);  // 设置有效期结束

        X509_NAME* name = X509_get_subject_name(cert);  // 获取主题名
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC,    // 添加国家
            (const unsigned char*)country.c_str(), -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,    // 添加组织
            (const unsigned char*)org.c_str(), -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,   // 添加通用名称
            (const unsigned char*)commonName.c_str(), -1, -1, 0);
        X509_set_issuer_name(cert, name);           // 自签名，颁发者=主题

        X509_set_pubkey(cert, pkey);                // 设置公钥

        if (!sanList.empty()) {                     // 有 SAN 列表
            GENERAL_NAMES* gens = sk_GENERAL_NAME_new_null();  // 创建 SAN 列表
            for (const auto& san : sanList) {       // 遍历 SAN
                GENERAL_NAME* gen = GENERAL_NAME_new();  // 创建 SAN 条目
                if (san.find(':') != std::string::npos || san.find('.') == std::string::npos) {
                    ASN1_OCTET_STRING* ip = ASN1_OCTET_STRING_new();  // IP 地址
                    if (san == "::1") {            // IPv6 本地回环
                        unsigned char ipv6[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
                        ASN1_OCTET_STRING_set(ip, ipv6, 16);
                    } else {                        // IPv4 地址
                        struct in_addr addr;        // IPv4 地址结构
                        inet_pton(AF_INET, san.c_str(), &addr);  // 转换 IP
                        ASN1_OCTET_STRING_set(ip, (const unsigned char*)&addr, 4);
                    }
                    GENERAL_NAME_set0_value(gen, GEN_IPADD, ip);  // 设置 IP 地址
                } else {
                    ASN1_IA5STRING* ia5 = ASN1_IA5STRING_new();  // 域名
                    ASN1_STRING_set(ia5, san.c_str(), san.length());  // 设置域名
                    GENERAL_NAME_set0_value(gen, GEN_DNS, ia5);  // 设置 DNS
                }
                sk_GENERAL_NAME_push(gens, gen);    // 添加到列表
            }
            X509_add_ext(cert, NID_subject_alt_name, gens);  // 添加 SAN 扩展
        }

        X509_sign(cert, pkey, EVP_sha256());        // 自签名

        FILE* keyFile = fopen(getKeyPath().c_str(), "wb");  // 打开私钥文件
        if (keyFile) {                              // 打开成功
            PEM_write_PrivateKey(keyFile, pkey, nullptr, nullptr, 0, nullptr, nullptr);  // 写入私钥
            fclose(keyFile);                        // 关闭文件
        }

        FILE* certFile = fopen(getCertPath().c_str(), "wb");  // 打开证书文件
        if (certFile) {                             // 打开成功
            PEM_write_X509(certFile, cert);         // 写入证书
            fclose(certFile);                       // 关闭文件
        }

        FILE* pemFile = fopen(getPemPath().c_str(), "wb");  // 打开 PEM 文件
        if (pemFile) {                              // 打开成功
            PEM_write_X509(pemFile, cert);          // 写入证书
            PEM_write_PrivateKey(pemFile, pkey, nullptr, nullptr, 0, nullptr, nullptr);  // 写入私钥
            fclose(pemFile);                        // 关闭文件
        }

        char buf[256];                              // 缓冲区
        BIO* bio = BIO_new(BIO_s_mem());            // 创建内存 BIO
        X509_NAME_print_ex(bio, X509_get_subject_name(cert), 0, XN_FLAG_RFC2253);  // 打印主题
        int len = BIO_read(bio, buf, sizeof(buf) - 1);  // 读取
        buf[len] = '\0';                            // 字符串结束
        info.issuer = buf;                          // 颁发者
        BIO_free(bio);                              // 释放 BIO

        bio = BIO_new(BIO_s_mem());                 // 创建内存 BIO
        ASN1_TIME_print(bio, X509_get_notBefore(cert));  // 打印有效期开始
        len = BIO_read(bio, buf, sizeof(buf) - 1);  // 读取
        buf[len] = '\0';                            // 字符串结束
        info.notBefore = buf;                       // 有效期开始
        BIO_free(bio);                              // 释放 BIO

        bio = BIO_new(BIO_s_mem());                 // 创建内存 BIO
        ASN1_TIME_print(bio, X509_get_notAfter(cert));   // 打印有效期结束
        len = BIO_read(bio, buf, sizeof(buf) - 1);  // 读取
        buf[len] = '\0';                            // 字符串结束
        info.notAfter = buf;                        // 有效期结束
        BIO_free(bio);                              // 释放 BIO

        BIGNUM* serialBN = ASN1_INTEGER_to_BN(X509_get_serialNumber(cert), nullptr);  // 序列号转大数
        char* serialHex = BN_bn2hex(serialBN);      // 大数转十六进制
        info.serialNumber = serialHex;              // 序列号
        OPENSSL_free(serialHex);                    // 释放
        BN_free(serialBN);                          // 释放大数

        unsigned char certDer[4096];                // DER 缓冲区
        int derLen = i2d_X509(cert, nullptr);       // 获取 DER 长度
        unsigned char* p = certDer;                 // 指针
        i2d_X509(cert, &p);                        // 编码为 DER
        info.fingerprint = sha256Hex(certDer, derLen);  // 计算指纹

        info.isCA = false;                          // 不是 CA 证书
        info.isExpired = false;                     // 未过期
        info.daysRemaining = days;                  // 剩余天数

        saveCertInfo(info);                         // 保存证书信息

        X509_free(cert);                            // 释放证书
        EVP_PKEY_free(pkey);                        // 释放密钥
        BN_free(bn);                                // 释放大数

        return info;                                // 返回证书信息
    }

    CertInfo CertManager::loadCertInfo() {          // 加载证书信息
        CertInfo info;                              // 证书信息
        std::ifstream infoFile(getInfoPath());      // 打开信息文件
        if (infoFile.good()) {                      // 文件存在
            std::string content((std::istreambuf_iterator<char>(infoFile)),
                                 std::istreambuf_iterator<char>());  // 读取全部内容
            auto getStr = [&](const std::string& key) -> std::string {  // 获取 JSON 字符串值
                size_t pos = content.find("\"" + key + "\"");  // 查找键
                if (pos == std::string::npos) return "";
                pos = content.find(":", pos);       // 查找冒号
                if (pos == std::string::npos) return "";
                pos = content.find("\"", pos);      // 查找引号
                if (pos == std::string::npos) return "";
                size_t end = content.find("\"", pos + 1);  // 查找结束引号
                if (end == std::string::npos) return "";
                return content.substr(pos + 1, end - pos - 1);  // 返回值
            };
            info.commonName = getStr("common_name");   // 通用名称
            info.org = getStr("org");                  // 组织
            info.country = getStr("country");          // 国家
            info.notBefore = getStr("not_before");     // 有效期开始
            info.notAfter = getStr("not_after");       // 有效期结束
            info.serialNumber = getStr("serial_number"); // 序列号
            info.fingerprint = getStr("fingerprint");  // 指纹
            info.issuer = getStr("issuer");            // 颁发者
            info.isCA = getStr("is_ca") == "true";    // 是否 CA
            info.isExpired = getStr("is_expired") == "true";  // 是否过期
            try {
                info.daysRemaining = std::stoi(getStr("days_remaining"));  // 剩余天数
            } catch (...) {
                info.daysRemaining = 0;                 // 解析失败
            }
        }

        std::ifstream certFile(getCertPath());      // 检查证书文件
        std::ifstream keyFile(getKeyPath());        // 检查私钥文件
        if (!certFile.good() || !keyFile.good()) {  // 文件缺失
            info.isExpired = true;                  // 标记为过期
            info.daysRemaining = 0;                 // 剩余天数为 0
        }

        return info;                                // 返回证书信息
    }

    CertInfo CertManager::checkAndRenew(            // 检查并续期
        int renewBeforeDays,                        // 提前续期天数
        const std::string& commonName,              // 通用名称
        const std::string& org,                     // 组织
        const std::string& country,                 // 国家
        const std::vector<std::string>& sanList,    // SAN 列表
        int days)                                   // 有效期
    {
        CertInfo info = loadCertInfo();             // 加载证书信息
        if (info.isExpired || info.daysRemaining < renewBeforeDays) {  // 需要续期
            return generateSelfSignedCert(commonName, org, country, sanList, days, true);  // 重新生成
        }
        return info;                                // 不需要续期
    }

    std::string CertManager::getCertPem() {         // 获取证书 PEM
        std::ifstream file(getCertPath());          // 打开证书文件
        if (!file.good()) return "";               // 文件不存在
        return std::string((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());  // 返回内容
    }

    std::string CertManager::getKeyPem() {          // 获取私钥 PEM
        std::ifstream file(getKeyPath());           // 打开私钥文件
        if (!file.good()) return "";               // 文件不存在
        return std::string((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());  // 返回内容
    }

    std::map<std::string, std::string> CertManager::getCertFiles() {  // 获取所有证书文件
        std::map<std::string, std::string> files;   // 文件映射
        files["server.key"] = getKeyPem();         // 私钥
        files["server.crt"] = getCertPem();        // 证书
        files["server.pem"] = [this]() {           // PEM
            std::ifstream file(getPemPath());       // 打开 PEM 文件
            if (!file.good()) return std::string(); // 文件不存在
            return std::string((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());  // 返回内容
        }();
        return files;                               // 返回文件映射
    }

    void CertManager::saveCertInfo(const CertInfo& info) {  // 保存证书信息
        std::ofstream file(getInfoPath());          // 打开信息文件
        if (file.good()) {                          // 打开成功
            file << info.toJson();                  // 写入 JSON
        }
    }

} // namespace Cert