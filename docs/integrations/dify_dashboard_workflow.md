# Dify 工作流接入模板（决策看板 AI 解读）

本模板用于把 `dify_orchestrated` profile 接入为可运行链路。

## 1. 应用类型

- Dify App Type: `Workflow`
- 建议应用名：`dashboard-insight-workflow`

## 2. 输入变量

在 Workflow 起始节点声明以下输入：

- `system_prompt` (text)
- `user_prompt` (text)
- `snapshot_json` (text)
- `focus` (text)
- `tone` (text)

## 3. 节点建议

1. `Template` 节点（可选）
- 合并 system/user 内容，形成统一提示。

2. `LLM` 节点（必选）
- Model Provider: OpenAI-compatible
- Base URL: `http://<litellm-host>:4000/v1`
- API Key: `LITELLM_MASTER_KEY`
- Model: `dashboard-default`
- Temperature: `0.2`

3. `JSON Parse` 节点（建议）
- 校验 LLM 输出是否为 JSON。
- 失败时走 fallback 分支，返回保底 JSON。

4. `End` 节点
- 输出键固定：`insight_json`

## 4. 输出格式要求

`insight_json` 必须是 JSON 字符串，结构建议：

```json
{
  "summary": "...",
  "highlights": ["..."],
  "risks": ["..."],
  "actions": ["...", "...", "..."],
  "confidence": "high",
  "evidence": {
    "major_count": 0,
    "total_students": 0,
    "employment_rate": 0,
    "alignment_rate": 0,
    "avg_excellent_score": 0,
    "top_gap_skill": "..."
  }
}
```

## 5. 后端 profile 对齐

确保 `backend/config/ai_agent_profiles.json` 的 `dify_orchestrated` 与 Dify 配置一致：

- `provider=dify_workflow`
- `endpoint=http://127.0.0.1:8080/v1/workflows/run`（或自定义为 `{env:DIFY_BASE_URL}/v1/workflows/run`）
- `dify_output_key=insight_json`

## 6. 调试建议

1. 先在 Dify 内部调通（手动输入 mock snapshot）。
2. 再从后端调用 `/api/dashboard/insight`，`profile_id=dify_orchestrated`。
3. 若后端提示 `llm endpoint returned status ...`，优先检查 Dify API key 与 Base URL。
