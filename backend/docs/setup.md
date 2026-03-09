# 后端启动说明

## 编译

```bash
cd /home/roamer/graduation_proj/backend
cmake -S . -B build
cmake --build build -j
```

## 启动

```bash
cd /home/roamer/graduation_proj/backend/build
./graduate_match_backend
```

可选：用环境变量覆盖数据库连接（避免在配置文件写死密码）

```bash
export GM_DB_HOST=127.0.0.1
export GM_DB_PORT=3306
export GM_DB_NAME=graduate_match
export GM_DB_USER=root
export GM_DB_PASS=123456
./graduate_match_backend
```

如需启用“邮箱找回密码（SMTP）”，至少配置以下环境变量：

```bash
export GM_SMTP_HOST=smtp.example.com
export GM_SMTP_PORT=587
export GM_SMTP_USERNAME=your_account@example.com
export GM_SMTP_PASSWORD=your_smtp_password
export GM_SMTP_FROM=your_account@example.com
export GM_SMTP_AUTH=LOGIN
export GM_SMTP_STARTTLS=true
# 可选：重置页面地址（邮件中展示）
export GM_PASSWORD_RESET_PAGE_URL=http://127.0.0.1:5173/password
# 可选：调试模式回传 token（默认 false，生产请保持关闭）
export GM_PASSWORD_RESET_DEBUG_RETURN_TOKEN=false
```

说明：SMTP 发送依赖系统 `curl` 命令，请先确认可用（`curl --version`）。

## 验证

```bash
curl http://127.0.0.1:5555/api/health
```

期望返回（示例）：

```json
{"status":"ok","service":"graduate_match_backend","version":"0.3.0"}
```

## 认证快速验证

默认测试账号（见 `db/seed_full.sql`）：

- 用户名：`admin`
- 密码：`admin123`

如需一键准备完整样本数据：

```bash
cd /home/roamer/graduation_proj
./db/load_full_seed.sh
```

```bash
curl -s -X POST http://127.0.0.1:5555/api/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"admin","password":"admin123"}'
```
