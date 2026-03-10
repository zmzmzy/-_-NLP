# 后端启动说明

本文件聚焦后端细节；完整项目启动说明请先看根目录 `README.md`。

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

## 数据库连接覆盖（可选）

默认读取 `backend/config/dev.json`。若需临时覆盖，请设置：

```bash
export GM_DB_HOST=127.0.0.1
export GM_DB_PORT=3306
export GM_DB_NAME=graduate_match
export GM_DB_USER=root
export GM_DB_PASS=123456
./graduate_match_backend
```

## SMTP（找回密码）配置

至少设置以下变量：

```bash
export GM_SMTP_HOST=smtp.example.com
export GM_SMTP_PORT=587
export GM_SMTP_USERNAME=your_account@example.com
export GM_SMTP_PASSWORD=your_smtp_auth_code
export GM_SMTP_FROM=your_account@example.com
export GM_SMTP_AUTH=LOGIN
export GM_SMTP_STARTTLS=true
export GM_SMTP_USE_SSL=false
export GM_SMTP_SKIP_TLS_VERIFY=false
export GM_SMTP_TIMEOUT_MS=10000
export GM_PASSWORD_RESET_PAGE_URL=http://127.0.0.1:5173/password
export GM_PASSWORD_RESET_DEBUG_RETURN_TOKEN=false
```

说明：

- SMTP 发信依赖系统 `curl`，请先确认 `curl --version` 可用
- Linux/Windows 可直接使用项目内示例文件：
  - `backend/.env.smtp.example`
  - `backend/.env.smtp.example.bat`

## 验证

```bash
curl http://127.0.0.1:5555/api/health
```

期望返回（示例）：

```json
{"status":"ok","service":"graduate_match_backend","version":"0.3.0"}
```

## 认证验证（当前策略）

- 系统不再内置默认账号
- 先通过 `/api/auth/register` 注册，再登录
- 需要管理员权限时，手动执行 SQL 提升角色（见 `README.md`）
