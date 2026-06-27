#!/usr/bin/env python3
# setup.py
# 一键部署：生成证书 + Nginx 配置

import subprocess       # 子进程
import os               # 文件系统
import sys              # 系统
from pathlib import Path  # 路径

def run_script(script_name, args):  # 运行子脚本
    script_path = Path(__file__).parent / script_name
    if not script_path.exists():
        print(f"[ERR] 脚本不存在: {script_path}")
        return False
    
    cmd = [sys.executable, str(script_path)] + args
    print(f"\n{'='*50}")
    print(f"  运行: {script_name}")
    print(f"{'='*50}")
    
    result = subprocess.run(cmd)
    return result.returncode == 0

def setup(  # 一键部署
    certs_dir="./certs",
    nginx_dir="./nginx",
    domain="localhost",
    http_port=80,
    https_port=443,
    backend_port=60906,
    days=3650,
    key_size=2048,
    common_name="localhost",
    org="Internal Dev",
    country="CN",
    san_list=None,
    https_only=False,
    static_dir="/usr/share/nginx/html",
    use_python=False
):
    print("=" * 50)
    print("  CertService 一键部署")
    print("=" * 50)
    print(f"  域名:      {domain}")
    print(f"  证书目录:  {certs_dir}")
    print(f"  Nginx 目录: {nginx_dir}")
    print(f"  后端端口:  {backend_port}")
    print("=" * 50)
    print()
    
    # 1. 生成证书
    print("[1/2] 生成 SSL 证书...")
    cert_args = [
        "--certs-dir", certs_dir,
        "--days", str(days),
        "--key-size", str(key_size),
        "--common-name", common_name,
        "--org", org,
        "--country", country,
    ]
    if san_list:
        cert_args.extend(["--san"] + san_list)
    
    if not run_script("gen_cert.py", cert_args):
        print("[ERR] 证书生成失败")
        return False
    print("[OK] 证书生成完成")
    print()
    
    # 2. 生成 Nginx 配置
    print("[2/2] 生成 Nginx 配置...")
    nginx_args = [
        "--nginx-dir", nginx_dir,
        "--domain", domain,
        "--http-port", str(http_port),
        "--https-port", str(https_port),
        "--backend-port", str(backend_port),
        "--certs-dir", certs_dir,
        "--static-dir", static_dir,
    ]
    if https_only:
        nginx_args.append("--https-only")
    if use_python:
        nginx_args.append("--use-python")
    
    if not run_script("gen_nginx_conf.py", nginx_args):
        print("[ERR] Nginx 配置生成失败")
        return False
    print("[OK] Nginx 配置生成完成")
    print()
    
    print("=" * 50)
    print("  部署完成！")
    print("=" * 50)
    print(f"  证书文件: {certs_dir}/")
    print(f"    - server.crt")
    print(f"    - server.key")
    print(f"    - server.pem")
    print(f"  Nginx 配置: {nginx_dir}/")
    print(f"    - nginx.conf")
    print("=" * 50)
    print()
    print("  后续步骤:")
    print(f"  1. 复制 {certs_dir}/ 到 Nginx SSL 目录")
    print(f"  2. 复制 {nginx_dir}/nginx.conf 到 /etc/nginx/nginx.conf")
    print("  3. 运行: sudo nginx -t && sudo nginx -s reload")
    print(f"  4. 访问: https://{domain} 验证")
    print()
    
    return True

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="一键部署证书和 Nginx 配置")
    parser.add_argument("--certs-dir", default="./certs", help="证书输出目录")
    parser.add_argument("--nginx-dir", default="./nginx", help="Nginx 配置输出目录")
    parser.add_argument("--domain", default="localhost", help="域名")
    parser.add_argument("--http-port", type=int, default=80, help="HTTP 端口")
    parser.add_argument("--https-port", type=int, default=443, help="HTTPS 端口")
    parser.add_argument("--backend-port", type=int, default=60906, help="后端 RPC 端口")
    parser.add_argument("--days", type=int, default=3650, help="证书有效期（天）")
    parser.add_argument("--key-size", type=int, default=2048, help="RSA 密钥长度")
    parser.add_argument("--common-name", default="localhost", help="通用名称 CN")
    parser.add_argument("--org", default="Internal Dev", help="组织 O")
    parser.add_argument("--country", default="CN", help="国家 C")
    parser.add_argument("--san", nargs="+", help="Subject Alternative Names")
    parser.add_argument("--https-only", action="store_true", help="仅 HTTPS 模式")
    parser.add_argument("--static-dir", default="/usr/share/nginx/html", help="静态文件目录")
    parser.add_argument("--use-python", action="store_true", help="使用纯 Python 生成（不调用 C++）")
    
    args = parser.parse_args()
    
    san_list = args.san if args.san else ["localhost", "127.0.0.1", "::1"]
    
    success = setup(
        certs_dir=args.certs_dir,
        nginx_dir=args.nginx_dir,
        domain=args.domain,
        http_port=args.http_port,
        https_port=args.https_port,
        backend_port=args.backend_port,
        days=args.days,
        key_size=args.key_size,
        common_name=args.common_name,
        org=args.org,
        country=args.country,
        san_list=san_list,
        https_only=args.https_only,
        static_dir=args.static_dir,
        use_python=args.use_python
    )
    
    sys.exit(0 if success else 1)