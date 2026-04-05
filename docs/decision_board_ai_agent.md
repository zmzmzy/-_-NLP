# 决策看板 AI 解读模块交付说明（v0.7）

## 1. 目标

在“决策看板”中交付可落地的 AI 解读能力，采用两层架构：

1. 第一层（必须）：`LiteLLM Gateway`
2. 第二层（可选增强）：`Dify Workflow 编排`

后端统一读取看板快照，调用可配置 Agent profile，输出结构化结论并落库。

## 2. 最终架构

### 标准链路（推荐）

`Frontend -> Backend(/api/dashboard/insight) -> LiteLLM -> OpenAI/OpenRouter/Gemini`

当前推荐默认模型路由为：`LiteLLM -> SiliconFlow(OpenAI-compatible)`。

### 二层编排链路（增强）

`Frontend -> Backend -> Dify Workflow -> LiteLLM -> OpenAI/OpenRouter/Gemini`

说明：

- LiteLLM 负责模型路由、fallback、统一鉴权入口。
- Dify 负责流程编排（变量清洗、策略分支、可视化工作流）。

## 3. 本次交付内容

### 后端

- `backend/src/ai_insight_agent.cc`
  - 新增 provider 支持：`litellm`、`dify_workflow`、`dify_chat`
  - endpoint 支持模板变量：`{model}` 与 `{env:ENV_NAME}`
  - 支持 Dify 响应提取（`answer` / `data.outputs` / 自定义 `dify_output_key`）
  - 错误可观测性增强：失败信息包含脱敏 endpoint 与上游 message
  - 支持 `api_key_required`（可选无密钥模式）
- `backend/config/ai_agent_profiles.json`
  - 默认 profile 改为 `litellm_guarded`
  - 新增 `dify_orchestrated` 作为第二层编排 profile
  - 保留 OpenAI/OpenRouter/Gemini 直连 fallback profile
- `backend/.env.ai.example`
  - 增加 LiteLLM / Dify 环境变量示例

### 网关部署资产

- `deploy/litellm/docker-compose.yml`
- `deploy/litellm/config.yaml(.example)`
- `deploy/litellm/.env.example`
- `tools/ai_stack/start_litellm.sh`
- `tools/ai_stack/stop_litellm.sh`
- `tools/ai_stack/README.md`

## 4. 关键 profile 字段（新增/扩展）

- `provider`:
  - `litellm`
  - `dify_workflow`
  - `dify_chat`
  - `openai_compatible`
  - `gemini`
- `endpoint`: 支持 `"{env:LITELLM_BASE_URL}/v1/chat/completions"` 形式
- `api_key_required`: 是否强制检查密钥
- `dify_user`: Dify 用户标识
- `dify_response_mode`: `blocking|streaming`
- `dify_output_key`: 工作流输出字段名（默认 `insight_json`）

## 5. 落地步骤（生产可执行）

### 步骤 A：启动 LiteLLM

```bash
cd /home/roamer/graduation_proj_0.1
cp deploy/litellm/.env.example deploy/litellm/.env
cp deploy/litellm/config.yaml.example deploy/litellm/config.yaml
# 编辑 .env / config.yaml，填入真实模型 key
./tools/ai_stack/start_litellm.sh
```

健康检查：

```bash
curl --noproxy '*' -H "Authorization: Bearer <LITELLM_MASTER_KEY>" \
  http://127.0.0.1:4000/v1/models
```

### 步骤 B：配置后端 AI 环境

```bash
cd /home/roamer/graduation_proj_0.1
cp backend/.env.ai.example backend/.env.ai
# 编辑 backend/.env.ai，至少填：
# LITELLM_BASE_URL / LITELLM_MASTER_KEY
```

### 步骤 C：启动项目

```bash
./start_linux.sh
```

### 步骤 D：接口验证

```bash
# 登录拿 token 后调用
curl -H "Authorization: Bearer <token>" \
  http://127.0.0.1:5555/api/dashboard/insight/profiles

curl -X POST -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"profile_id":"litellm_guarded","scope":"all"}' \
  http://127.0.0.1:5555/api/dashboard/insight
```

## 6. Dify 第二层编排接入

### Dify 侧建议配置

1. 在 Dify 新建 Workflow 应用（建议命名：`dashboard-insight-workflow`）
2. 定义输入变量：
- `system_prompt`（text）
- `user_prompt`（text）
- `snapshot_json`（text）
- `focus`（text）
- `tone`（text）
3. LLM 节点模型改为 LiteLLM OpenAI-compatible：
- Base URL: `http://<litellm-host>:4000/v1`
- API Key: `LITELLM_MASTER_KEY`
- Model: `dashboard-default`
4. 最终输出字段固定为：`insight_json`（JSON 字符串）
5. 在后端 profile 中选择 `dify_orchestrated`

### 后端 profile 对应字段

- `provider`: `dify_workflow`
- `endpoint`: `http://127.0.0.1:8080/v1/workflows/run`（也可改成 `{env:DIFY_BASE_URL}/v1/workflows/run`）
- `dify_output_key`: `insight_json`

## 7. 常见故障与排查

1. `llm request failed: bad_server_address`
- endpoint 缺少协议或地址不可解析。
- 检查 `LITELLM_BASE_URL` / `DIFY_BASE_URL` 是否为完整 URL（含 `http://`）。

2. `llm request failed: network_failure (http://127.0.0.1:4000/...)`
- 常见根因：系统开启了 `HTTP_PROXY/HTTPS_PROXY`，但未配置 `NO_PROXY`，导致本机请求被代理拦截。
- 修复：
```bash
export NO_PROXY=127.0.0.1,localhost,::1,0.0.0.0
export no_proxy=127.0.0.1,localhost,::1,0.0.0.0
```
- 然后验证：
```bash
curl --noproxy '*' -H "Authorization: Bearer <LITELLM_MASTER_KEY>" \
  http://127.0.0.1:4000/v1/models
```

3. `llm endpoint returned status 401`
- 网关密钥不匹配。
- 确认后端 `LITELLM_MASTER_KEY` 与 LiteLLM `.env` 一致。

4. `llm endpoint returned status 429`
- 上游模型额度不足。
- 在 LiteLLM `fallbacks` 中配置可用备用模型或切换供应商。

5. 国内网络下 OpenAI 连接失败
- 错误特征：`OpenAIException - Connection error` 或 `Network is unreachable`。
- 建议优先改用 `SILICONFLOW_API_KEY`，并使用 `siliconflow_direct` 或 `litellm_guarded`（默认已切 SiliconFlow）。

6. 前端 `http proxy error: socket hang up`
- 后端请求超时或崩溃。
- 已将前端代理超时放宽，仍需检查后端日志和网关可达性。

7. 如何查看“发给模型的内容”和“模型原始返回”
- `dashboard_insight_runs` 已持久化 `prompt_text` 和 `raw_response_text`。
- 直接通过接口读取（携带 `include_payload=1`）：
```bash
curl -H "Authorization: Bearer <token>" \
  "http://127.0.0.1:5555/api/dashboard/insight/runs?page=1&page_size=1&include_payload=1"
```

## 8. 安全说明

- 不要把真实 key 写入仓库。
- 已移除示例文件中的真实 Gemini key，统一改为占位符。
- 建议生产环境用密钥管理系统注入（CI/CD Secret / Vault）。
