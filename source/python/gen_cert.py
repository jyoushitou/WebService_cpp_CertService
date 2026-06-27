#!/usr/bin/env python3
# gen_cert.py
# 调用 OpenSSL 命令行生成自签名证书

import subprocess       # 子进程
import os               # 文件系统
import sys              # 系统
from pathlib import Path  # 路径

def run_cmd(cmd):  # 运行命令
    print(f"[CMD] {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[ERR] {result.stderr}")
        return False
    if result.stdout:
        print(result.stdout)
    return True

def gen_cert(  # 生成证书
    certs_dir="./certs",  # 证书目录
    days=3650,  # 有效期
    key_size=2048,  # 密钥长度
    common_name="localhost",  # 通用名称
    org="Internal Dev",  # 组织
    country="CN",  # 国家
    san_list=None  # SAN 列表
):
    if san_list is None:
        san_list = ["localhost", "127.0.0.1", "::1"]
    
    os.makedirs(certs_dir, exist_ok=True)
    
    key_path = os.path.join(certs_dir, "server.key")  # 私钥路径
    cert_path = os.path.join(certs_dir, "server.crt")  # 证书路径
    pem_path = os.path.join(certs_dir, "server.pem")  # PEM 路径
    
    # 1. 生成 RSA 私钥
    if not run_cmd(["openssl", "genrsa", "-out", key_path, str(key_size)]):
        return False
    
    # 2. 生成 SAN 配置文件
    san_conf = os.path.join(certs_dir, "san.conf")
    with open(san_conf, "w") as f:
        f.write("[req]\n")
        f.write("distinguished_name = req_distinguished_name\n")
        f.write("req_extensions = v3_req\n")
        f.write("prompt = no\n\n")
        f.write("[req_distinguished_name]\n")
        f.write(f"C = {country}\n")
        f.write(f"O = {org}\n")
        f.write(f"CN = {common_name}\n\n")
        f.write("[v3_req]\n")
        f.write("keyUsage = keyEncipherment, dataEncipherment\n")
        f.write("extendedKeyUsage = serverAuth\n")
        f.write("subjectAltName = @alt_names\n\n")
        f.write("[alt_names]\n")
        dns_idx = 1
        ip_idx = 1
        for san in san_list:
            if ":" in san or san.replace(".", "").isdigit():
                f.write(f"IP.{ip_idx} = {san}\n")
                ip_idx += 1
            else:
                f.write(f"DNS.{dns_idx} = {san}\n")
                dns_idx += 1
    
    # 3. 生成自签名证书
    if not run_cmd([
        "openssl", "req", "-new", "-x509",
        "-key", key_path,
        "-out", cert_path,
        "-days", str(days),
        "-config", san_conf
    ]):
        return False
    
    # 4. 生成 PEM（证书 + 私钥）
    with open(pem_path, "wb") as pem_f:
        with open(cert_path, "rb") as cert_f:
            pem_f.write(cert_f.read())
        with open(key_path, "rb") as key_f:
            pem_f.write(key_f.read())
    
    os.remove(san_conf)
    
    print(f"[OK] 证书已生成: {cert_path}")
    print(f"[OK] 私钥已生成: {key_path}")
    print(f"[OK] PEM 已生成: {pem_path}")
    return True

def check_cert(certs_dir="./certs"):  # 检查证书
    cert_path = os.path.join(certs_dir, "server.crt")
    key_path = os.path.join(certs_dir, "server.key")
    
    if not os.path.exists(cert_path) or not os.path.exists(key_path):
        print("[WARN] 证书文件不存在")
        return False
    
    result = subprocess.run(["openssl", "x509", "-noout", "-modulus", "-in", cert_path],
                           capture_output=True, text=True)
    cert_mod = result.stdout.strip()
    
    result = subprocess.run(["openssl", "rsa", "-noout", "-modulus", "-in", key_path],
                           capture_output=True, text=True)
    key_mod = result.stdout.strip()
    
    if cert_mod != key_mod:
        print("[WARN] 证书与私钥不匹配")
        return False
    
    result = subprocess.run(["openssl", "x509", "-checkend", "0", "-in", cert_path],
                           capture_output=True, text=True)
    if "not" in result.stdout:
        print("[WARN] 证书已过期")
        return False
    
    print("[OK] 证书有效")
    return True

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="生成自签名 SSL 证书")
    parser.add_argument("--certs-dir", default="./certs", help="证书输出目录")
    parser.add_argument("--days", type=int, default=3650, help="证书有效期（天）")
    parser.add_argument("--key-size", type=int, default=2048, help="RSA 密钥长度")
    parser.add_argument("--common-name", default="localhost", help="通用名称 CN")
    parser.add_argument("--org", default="Internal Dev", help="组织 O")
    parser.add_argument("--country", default="CN", help="国家 C")
    parser.add_argument("--san", nargs="+", help="Subject Alternative Names")
    parser.add_argument("--check", action="store_true", help="检查证书")
    
    args = parser.parse_args()
    
    if args.check:
        sys.exit(0 if check_cert(args.certs_dir) else 1)
    
    san_list = args.san if args.san else ["localhost", "127.0.0.1", "::1"]
    
    if gen_cert(
        certs_dir=args.certs_dir,
        days=args.days,
        key_size=args.key_size,
        common_name=args.common_name,
        org=args.org,
        country=args.country,
        san_list=san_list
    ):
        sys.exit(0)
    else:
        sys.exit(1)