// CertInfo.cpp
// 证书信息结构体实现

#include "../include/CertInfo.h" // 头文件

namespace Cert {

    std::string CertInfo::toJson() const {  // 序列化为 JSON
        std::ostringstream json;            // JSON 字符串流
        json << "{\n";                     // 开始 JSON 对象
        json << "  \"common_name\": \"" << commonName << "\",\n";  // 通用名称
        json << "  \"org\": \"" << org << "\",\n";                  // 组织
        json << "  \"country\": \"" << country << "\",\n";          // 国家
        json << "  \"san_list\": [";      // SAN 列表开始
        for (size_t i = 0; i < sanList.size(); ++i) {   // 遍历 SAN
            if (i > 0) json << ", ";       // 逗号分隔
            json << "\"" << sanList[i] << "\"";  // SAN 值
        }
        json << "],\n";                    // SAN 列表结束
        json << "  \"not_before\": \"" << notBefore << "\",\n";    // 有效期开始
        json << "  \"not_after\": \"" << notAfter << "\",\n";      // 有效期结束
        json << "  \"serial_number\": \"" << serialNumber << "\",\n"; // 序列号
        json << "  \"fingerprint\": \"" << fingerprint << "\",\n"; // 指纹
        json << "  \"issuer\": \"" << issuer << "\",\n";          // 颁发者
        json << "  \"is_ca\": " << (isCA ? "true" : "false") << ",\n"; // 是否 CA
        json << "  \"is_expired\": " << (isExpired ? "true" : "false") << ",\n"; // 是否过期
        json << "  \"days_remaining\": " << daysRemaining << "\n";   // 剩余天数
        json << "}";                        // 结束 JSON 对象
        return json.str();                   // 返回 JSON 字符串
    }

} // namespace Cert