# 文档索引与完成度检查

日期：2026-03-02

## 建议阅读顺序

1. `docs/requirements_baseline.md`：先看导师需求如何落地为系统需求。
2. `docs/architecture.md`：看分层和数据流。
3. `docs/ordered_todo.md`：看项目路线和当前里程碑。
4. `docs/functional_design_flowchart.md`：看功能模块端到端流程图。
5. `docs/defense_simplified_flowchart.md`：答辩展示用简化业务流程图。
6. `backend/docs/api_contract.md`：看接口设计与字段约定。
7. `docs/testing_guide.md`：按命令验收当前完成度。

## 完成度快照（便于复盘）

- 阶段 0：需求与范围冻结（文档已完成，待你与导师最终签字确认）
- 阶段 1：数据基础（已完成，含 `seed_full.sql` 与验收脚本）
- 阶段 2：后端核心 API（已完成，包括认证与系统日志）
- 阶段 3：匹配引擎基线（已完成）
- 阶段 4：前端核心页面（已完成）
- 阶段 5：实验与论文证据（未开始）
- 阶段 6：收敛与交付（未开始）

## 文档检查清单（你可每周自检）

- 需求是否与导师最新要求一致（看 `requirements_baseline.md`）
- 接口文档是否与代码一致（对照 `api_contract.md` + 实际 curl）
- TODO 勾选是否基于“可运行验证”而不是“口头完成”（看 `ordered_todo.md`）
- 是否有可复现实验记录（后续补到 `docs/`）
- 论文图表是否对应系统真实数据（后续阶段 5）
