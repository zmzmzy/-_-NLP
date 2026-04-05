# AI Stack Scripts

## LiteLLM 网关

启动：

```bash
cd /home/roamer/graduation_proj_0.1
# 可选：先在 backend/.env.ai 填好 SILICONFLOW_API_KEY（脚本会自动同步到 deploy/litellm/.env）
./tools/ai_stack/start_litellm.sh
```

停止：

```bash
cd /home/roamer/graduation_proj_0.1
./tools/ai_stack/stop_litellm.sh
```

## 目录说明

- `deploy/litellm/docker-compose.yml`：LiteLLM 容器编排
- `deploy/litellm/config.yaml`：LiteLLM 路由配置（可按需调整模型与 fallback）
- `deploy/litellm/.env`：LiteLLM 密钥与端口配置

## 常见问题

1. `401 unauthorized`
- 检查 `LITELLM_MASTER_KEY` 与后端 `backend/.env.ai` 的 `LITELLM_MASTER_KEY` 是否一致。

2. `llm request failed: network_failure (http://127.0.0.1:4000/...)`
- 常见原因是系统设置了 `HTTP_PROXY/HTTPS_PROXY`，但没有 `NO_PROXY`，导致本机地址被转发到代理。
- 先执行：
```bash
export NO_PROXY=127.0.0.1,localhost,::1,0.0.0.0
export no_proxy=127.0.0.1,localhost,::1,0.0.0.0
```
- 再检查网关：
```bash
curl --noproxy '*' -H "Authorization: Bearer <LITELLM_MASTER_KEY>" http://127.0.0.1:4000/v1/models
```

3. `llm request failed: bad_server_address`
- 检查 `LITELLM_BASE_URL` 是否包含协议（例如 `http://127.0.0.1:4000`）。
- 检查网关容器是否启动：`docker ps | grep gm-litellm`。

4. 上游模型 429 / quota
- 在 `deploy/litellm/config.yaml` 中配置多模型 fallback，或切换到额度更稳定的供应商。

5. 国内网络访问 OpenAI 超时
- 默认 `dashboard-default` 已切到 SiliconFlow（`https://api.siliconflow.cn/v1`）。
- 在 `deploy/litellm/.env` 填写 `SILICONFLOW_API_KEY` 后重启网关。

6. 想看“发送给模型的内容”和“模型原始返回”
- 后端会把 `prompt_text` 与 `raw_response_text` 存到 `dashboard_insight_runs`。
- 可通过接口查看（`include_payload=1`）：
```bash
curl -H "Authorization: Bearer <token>" \
  "http://127.0.0.1:5555/api/dashboard/insight/runs?page=1&page_size=1&include_payload=1"
```
