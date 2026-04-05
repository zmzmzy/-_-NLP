# 文档索引与完成度检查


## 建议阅读顺序

1. `docs/requirements_baseline.md`：先看导师需求如何落地为系统需求。
2. `README.md`：先跑通环境、部署与运行流程（前端/后端/数据库注意事项）。
3. `docs/requirements_baseline_v0.4.md`：当前主线需求冻结版本（学生就业-专业对口）。
4. `docs/architecture.md`：看分层和数据流。
5. `docs/db_table_flowchart.md`：数据库表关系 Flowchart（Mermaid）。
6. `docs/refactor_todolist_v0.4.md`：学生就业-专业对口主线重构清单（当前执行基线）。
7. `docs/ordered_todo_v0.2.md`：v0.2 重构与优化执行清单（历史记录，含已下线能力）。
8. `docs/ordered_todo.md`：v0.1 路线和历史里程碑。
9. `docs/functional_design_flowchart.md`：看功能模块端到端流程图。
10. `docs/defense_simplified_flowchart.md`：答辩展示用简化业务流程图。
11. `backend/docs/api_contract.md`：看接口设计与字段约定。
12. `backend/docs/setup.md`：后端单独启动、DB 覆盖与 SMTP 变量说明。
13. `frontend/docs/setup.md`：前端启动、代理与会话联调说明。
14. `docs/testing_guide.md`：按命令验收当前完成度。
15. `docs/testdata_pack_manual_v1.md`：备用测试数据导入与回滚手册。
16. `docs/defense_showcase_manual_v1.md`：答辩展示数据包导入、验收与回滚手册。
17. `tools/job_sync/README.md`：历史方案文档（已下线，不再作为当前主流程）。
18. `docs/db_replan_v0.3.md`：看数据库分层收敛方案（Core / Extension / Ops）。
19. `docs/decision_board_ai_agent.md`：决策看板 AI 解读（LiteLLM 主链路 + Dify 编排）交付说明。
20. `docs/integrations/dify_dashboard_workflow.md`：Dify 工作流接入模板与输出规范。

## 完成度快照（便于复盘）

v0.4 重构主线（2026-03-06）：

- 当前执行基线：`docs/refactor_todolist_v0.4.md`
- 当前阶段：R2（后端查询与聚合接口补强）

- 阶段 0：需求与范围冻结（文档已完成，待你与导师最终签字确认）
- 阶段 1：数据基础（已完成，含 `seed_full.sql` 与验收脚本）
- 阶段 2：后端核心 API（已完成，包括认证与系统日志）
- 阶段 3：匹配引擎基线（已完成）
- 阶段 4：前端核心页面（已完成）
- 阶段 5：实验与论文证据（未开始）
- 阶段 6：收敛与交付（未开始）

v0.2 增量（2026-03-04，历史）：

- 已完成：鉴权收口、数据库迁移脚本、专业导入 API、前端导入入口。
- 已下线：岗位同步 API 与同步入口（按当前课题范围做减法收敛）。

v0.3 预备（2026-03-04）：

- 已新增数据库重规划文档：`docs/db_replan_v0.3.md`
- 已新增主流程匹配结果表迁移草案：`db/migrations/v0.3_001_core_match_results.sql`

## 文档检查清单（自检）

- 需求是否与导师最新要求一致（看 `requirements_baseline_v0.4.md`）
- 接口文档是否与代码一致（对照 `api_contract.md` + 实际 curl）
- TODO 勾选是否基于“可运行验证”而不是“口头完成”（看 `ordered_todo.md`）
- 是否有可复现实验记录（后续补到 `docs/`）
- 论文图表是否对应系统真实数据（后续阶段 5）
