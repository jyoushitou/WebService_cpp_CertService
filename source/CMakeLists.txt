# CMakeLists.txt
# 证书微服务构建配置

cmake_minimum_required(VERSION 3.14)
project(CertService VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)          # OpenSSL：证书生成
find_package(Boost REQUIRED COMPONENTS beast asio system)  # Boost：HTTP 服务器
find_package(nlohmann_json QUIET)       # nlohmann/json：JSON 解析（可选）

set(CERT_SOURCES
    src/CertInfo.cpp            # 证书信息结构体
    src/CertManager.cpp         # 证书管理器
    src/NginxConfigGen.cpp      # Nginx 配置生成器
    src/HttpServer.cpp          # HTTP 服务器
    src/CertService.cpp         # 微服务主类
)

set(CERT_HEADERS
    include/CertConfig.h        # 配置结构体
    include/CertInfo.h          # 证书信息结构体
    include/CertManager.h       # 证书管理器
    include/NginxConfigGen.h    # Nginx 配置生成器
    include/HttpServer.h        # HTTP 服务器
    include/CertService.h       # 微服务主类
)

add_executable(${PROJECT_NAME}
    ${CERT_SOURCES}
    ${CERT_HEADERS}
    main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # 头文件目录
    ${OpenSSL_INCLUDE_DIR}               # OpenSSL 头文件
)

if(nlohmann_json_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${nlohmann_json_INCLUDE_DIRS}
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        HAS_NLOHMANN_JSON
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenSSL::SSL        # OpenSSL SSL
    OpenSSL::Crypto     # OpenSSL 加密
    Boost::beast        # Boost.Beast HTTP
    Boost::asio         # Boost.Asio 网络
    Boost::system       # Boost.System 错误码
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3             # 警告级别 3
        /MP             # 并行编译
        /wd4996         # 禁用安全警告
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-deprecated-declarations
    )
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

add_custom_target(generate_cert
    COMMAND ${CMAKE_COMMAND} -E echo "证书将在首次运行时自动生成"
    COMMENT "证书微服务构建完成，运行 ${PROJECT_NAME} 以生成证书"
)

message(STATUS "========================================")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "  OpenSSL:     ${OpenSSL_VERSION}")
message(STATUS "  Boost:       ${Boost_VERSION}")
message(STATUS "  C++ 标准:    ${CMAKE_CXX_STANDARD}")
message(STATUS "  构建类型:    ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")