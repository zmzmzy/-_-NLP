# 数据库重规划方案（v0.3 草案）

日期：2026-03-04  
适用系统：`graduation_proj`

## 1. 结论

- 核心层：专业数据、岗位数据、专业-岗位匹配
- 扩展层：学生就业追踪
- 支撑层：导入日志、系统操作日志

## 2. 分层结果

## 2.1 核心层（Core）

- `universities`
- `colleges`
- `majors`
- `major_profiles`
- `companies`
- `jobs`
- `skills`
- `major_skills`
- `job_skills`
- `major_job_match_results`（v0.3 新增）

## 2.2 扩展层（Extension）

- `students`
- `employment_records`
- `match_results`
- `major_alignment_snapshots`

说明：扩展层不删除，但不作为主流程依赖。

## 2.3 支撑层（Ops/Audit）

- `users`
- `system_logs`
- `major_import_batches`
- `major_import_rows`
- `job_import_batches`
- `job_import_rows`

## 3. 结构问题与修正方向

1. 问题：`match_results` 强依赖 `student_id`，无法承载“专业-岗位主匹配历史”。
- 修正：新增 `major_job_match_results`，主流程历史写新表。

2. 问题：删除学院会被学生/专业外键链路阻塞。
- 修正：已支持 `force=true` 管理员强制删除（事务级联清理）。

3. 问题：表名没有表达层级意图。
- 修正：文档层面标记 Core/Extension/Ops；代码层默认只走 Core。

## 4. v0.3 最小迁移步骤（建议）

1. 新增 `major_job_match_results` 表（不影响现有逻辑）。
2. 新增接口：
- `GET /api/match/major-job/results`
- （可选）`DELETE /api/match/major-job/results/{id}`
3. 前端匹配页历史查询改查新表。
4. 将学生就业页面标记为“扩展模块”。
5. 论文与答辩只描述 Core 主链路。

## 5. 风险控制

- 不删旧表：避免历史数据丢失。
- 所有迁移脚本幂等：`CREATE TABLE IF NOT EXISTS`。
- 先灰度读新表，再考虑是否归档旧 `match_results`。
- 岗位同步相关能力已下线，历史同步表不再作为当前版本依赖。

