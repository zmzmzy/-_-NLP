# 后端启动说明

本文件聚焦后端细节；完整项目启动说明请先看根目录 `README.md`。

## 编译

```bash
cd /home/roamer/graduation_proj_0.1/backend
cmake -S . -B build
cmake --build build -j
```

## 启动

```bash
cd /home/roamer/graduation_proj_0.1/backend/build
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

## 决策看板 AI 解读配置（v0.7）

后端默认读取：

- `backend/config/ai_agent_profiles.json`
- 其中 `rag.knowledge_file` 默认可指向 `backend/config/ai_rag_knowledge.json`

可通过环境变量覆盖配置路径：

```bash
export GM_AI_AGENT_CONFIG_PATH=/abs/path/to/ai_agent_profiles.json
```

推荐变量（LiteLLM 主链路）：

```bash
export LITELLM_BASE_URL=http://127.0.0.1:4000
export LITELLM_MASTER_KEY=...
```

可选变量（Dify 编排层）：

```bash
export DIFY_BASE_URL=http://127.0.0.1:8080
export DIFY_API_KEY=...
```

直连兜底变量（网关不可用时）：

```bash
export SILICONFLOW_API_KEY=...
export OPENAI_API_KEY=...
export OPENROUTER_API_KEY=...
export GEMINI_API_KEY=...
```

示例文件：`backend/.env.ai.example`

若本机使用内置 LiteLLM 部署，可先执行：

```bash
cd /home/roamer/graduation_proj_0.1
./tools/ai_stack/start_litellm.sh
```

说明：

- 至少保证 `ai_agent_profiles.json` 中 `default_profile` 可用
- 默认建议使用 `litellm_guarded`，模型别名为 `dashboard-default`
- 建议优先配置 `SILICONFLOW_API_KEY`，国内网络稳定性通常更好
- 若启用 Dify，建议使用 `dify_orchestrated` profile，并在 Dify workflow 输出 `insight_json`
- 不同 profile 可配置不同模型、约束（world book / preset / role card）和 RAG 参数
- 未配置对应密钥时，`POST /api/dashboard/insight` 会返回可读错误并记录失败 run
- 若出现 `bad_server_address`，优先检查 `LITELLM_BASE_URL/DIFY_BASE_URL` 是否含协议头
- 若出现 `network_failure` 访问 `127.0.0.1`，检查是否配置了：
```bash
export NO_PROXY=127.0.0.1,localhost,::1,0.0.0.0
export no_proxy=127.0.0.1,localhost,::1,0.0.0.0
```

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
