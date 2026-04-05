# API 合同草案（v0.5）

基础前缀：`/api`

鉴权方式：

- 登录后获得 `token`
- 受保护接口通过请求头携带：`Authorization: Bearer <token>`
- 公开接口仅保留：
  - `GET /health`
  - `POST /auth/login`
  - `POST /auth/register`
  - `POST /auth/password/forgot`
  - `POST /auth/password/reset`
- 其余 `/api/*` 接口默认都需要登录态

## 列表接口通用查询参数（已实现）

- `page`：页码，默认 `1`
- `page_size`：每页数量，默认 `20`，最大 `100`
- `sort_by`：排序字段（各接口白名单）
- `sort_order`：`asc` 或 `desc`
- `keyword`：关键词检索（按接口定义匹配字段）

列表返回统一包含：

- `data`
- `count`
- `page`
- `page_size`
- `total_pages`

## 错误响应格式（已实现）

错误响应统一为：

```json
{
  "success": false,
  "error": "错误信息",
  "error_code": "validation_error | database_error | api_error",
  "error_detail": {
    "code": "validation_error",
    "message": "错误信息"
  }
}
```

## 系统

- `GET /health`

## 认证（已实现）

- `POST /auth/login`
  - 请求体：`password` + 任选其一：`account` / `email` / `username`
  - 返回：`token`、`token_type`、`expires_at_epoch`、`user`
- `POST /auth/register`
  - 请求体：`email`、`password`，可选：`username`
  - 说明：默认注册角色为 `viewer`，并自动登录返回 token
- `POST /auth/password/forgot`
  - 请求体：`email`
  - 说明：通过 SMTP 发送 6 位重置验证码到邮箱，接口不返回验证码
- `POST /auth/password/reset`
  - 请求体：`email`、`token`、`new_password`
  - 说明：`token` 为邮件中的 6 位数字验证码
- `POST /auth/logout`
  - 说明：需要登录态，注销当前 token
- `GET /auth/me`
  - 说明：需要登录态，返回当前会话用户信息

## 学院（已实现）

- `GET /colleges`
- `POST /colleges`
- `GET /colleges/{id}`
- `PUT /colleges/{id}`
- `DELETE /colleges/{id}`
  - 可选查询参数：`force=true|false`
  - 默认行为：若存在关联专业/学生，返回 `409`
  - `force=true`：仅管理员可用，事务内级联清理关联数据后删除学院

## 专业（已实现）

- `GET /majors`
- `POST /majors`
- `GET /majors/{id}`
- `PUT /majors/{id}`
- `DELETE /majors/{id}`

## 专业画像（已实现）

- `GET /major-profiles/{majorId}`
- `PUT /major-profiles/{majorId}`

## 企业（已实现）

- `GET /companies`
- `POST /companies`
- `GET /companies/{id}`
- `PUT /companies/{id}`
- `DELETE /companies/{id}`

## 岗位（已实现）

- `GET /jobs`
  - 筛选参数：`company_id`、`min_degree`、`job_type`
- `POST /jobs`
- `GET /jobs/{id}`
- `PUT /jobs/{id}`
- `DELETE /jobs/{id}`

## 专业导入（v0.2，已实现）

- `POST /majors/import/preview`
  - 说明：管理员接口（需要 `admin`）
  - 入参方式二选一：
    - `csv_text`（CSV文本，推荐）
    - `rows`（对象数组，手动构造）
  - 可选：`source_type`（`csv|xlsx|manual`）、`source_filename`
  - 返回：`batch_id`、预检统计（`valid_rows/invalid_rows/estimated_*`）和逐行预览
- `POST /majors/import/apply`
  - 说明：管理员接口（需要 `admin`）
  - 请求体：`batch_id`
  - 行为：对 `preview` 状态批次执行事务导入，写入 `universities/colleges/majors/major_profiles`
  - 限制：若批次 `valid_rows=0`，拒绝应用并返回 `409`（需先修复 CSV）
- `GET /majors/import/batches`
  - 说明：管理员接口（需要 `admin`）
  - 查询参数：`page`、`page_size`、`sort_by`、`sort_order`
  - 筛选参数：`status`（`preview|applied|failed`）
  - 返回：导入批次历史与统计

## 岗位导入（v0.3，已实现）

- `POST /jobs/import/preview`
  - 说明：管理员接口（需要 `admin`）
  - 入参方式二选一：
    - `csv_text`（CSV文本，推荐）
    - `rows`（对象数组，手动构造）
  - 可选：`source_type`（`csv|xlsx|manual`）、`source_filename`
  - 返回：`batch_id`、预检统计（`valid_rows/invalid_rows/estimated_*`）和逐行预览
- `POST /jobs/import/apply`
  - 说明：管理员接口（需要 `admin`）
  - 请求体：`batch_id`
  - 行为：对 `preview` 状态批次执行事务导入，写入 `companies/jobs`
  - 限制：若批次 `valid_rows=0`，拒绝应用并返回 `409`（需先修复 CSV）
- `GET /jobs/import/batches`
  - 说明：管理员接口（需要 `admin`）
  - 查询参数：`page`、`page_size`、`sort_by`、`sort_order`
  - 筛选参数：`status`（`preview|applied|failed`）
  - 返回：导入批次历史与统计

## 学生（已实现）

- `GET /students`
- `POST /students`
- `GET /students/{id}`
- `PUT /students/{id}`
- `DELETE /students/{id}`

## 就业记录（已实现）

- `GET /employment-records`
- `POST /employment-records`
- `GET /employment-records/{id}`
- `PUT /employment-records/{id}`
- `DELETE /employment-records/{id}`

## 个人主页与就业审核（v0.5，已实现）

- `GET /me/student-profile`
  - 说明：返回当前用户信息 + 关联学生资料（若未绑定则 `student=null`）
- `POST /me/student-profile/bind`
  - 说明：普通用户使用 `student_no + full_name` 绑定已有学生档案；一个学生仅允许绑定一个账号
  - 请求体：`student_no`、`full_name`
- `PUT /me/student-profile`
  - 说明：普通用户维护已绑定学生的可编辑字段（`gender/email/phone/resume_text/technical_projects_text/capability_summary_text`）
  - 限制：未绑定时会返回 `409`；`student_no/full_name/college_id/major_id/graduation_year` 为绑定后不可改
- `GET /me/employment-submissions`
  - 查询参数：`page`、`page_size`、`sort_order`、`status`
  - 说明：普通用户查看自己的就业提交历史
- `POST /me/employment-submissions`
  - 请求体（至少满足其一）：`company_id`（库内公司）或 `proposed_company_name`（候选公司）
  - 请求体（可选）：
    - 现有实体：`job_id`
    - 候选公司：`proposed_company_main_business`、`proposed_company_flagship_products`、`proposed_company_domain_area`、`proposed_company_application_industries`、`proposed_company_province`、`proposed_company_city`
    - 候选岗位：`proposed_job_title`、`proposed_job_type`、`proposed_job_min_degree`、`proposed_job_salary_range`、`proposed_job_location_province`、`proposed_job_location_city`、`proposed_job_responsibilities_text`、`proposed_job_technical_projects_text`、`proposed_job_required_knowledge_skills_text`、`proposed_job_required_tools_platform_text`
    - 通用：`employment_type`、`employment_status`、`start_date`、`end_date`、`evidence_source`、`notes`
  - 说明：提交后状态为 `pending`；管理员审核通过时会自动去重并创建正式 `companies/jobs`（若需要），再写入 `employment_records`

- `GET /admin/employment-submissions`
  - 说明：管理员分页查询审核列表
  - 查询参数：`page`、`page_size`、`sort_order`、`status`、`keyword`
- `POST /admin/employment-submissions/{id}/approve`
  - 请求体可选：`review_comment`
  - 说明：通过后自动写入 `employment_records`
- `POST /admin/employment-submissions/{id}/reject`
  - 请求体：`review_comment`
- `POST /admin/employment-submissions/batch-approve`
  - 请求体：`ids[]`，可选 `review_comment`
- `POST /admin/employment-submissions/batch-reject`
  - 请求体：`ids[]`，可选 `review_comment`
- `GET /admin/employment-submissions/export`
  - 查询参数：`status`、`keyword`、`sort_order`、`limit`
  - 说明：按筛选条件导出 CSV

## 匹配（已实现）

- `POST /match/major-job`
  - 说明：主流程接口（专业-岗位匹配）
  - 请求体（必填）：`major_id`、`job_id`
  - 请求体（可选）：`algorithm_version`（默认 `v0.2-major-job`）、`persist`（默认 `false`）
  - 返回：`semantic_score`、`skill_score`、`constraint_score`、`final_score`、解释字段（`matched_skills` / `missing_skills` / `constraint_failures`）
- `POST /match/major-job/batch`
  - 说明：主流程批量接口（专业集 × 岗位集）
  - 请求体（必填）：`major_ids`、`job_ids`（数组）
  - 请求体（可选）：`algorithm_version`（默认 `v0.2-major-job`）、`persist`（默认 `false`）
  - 说明：按专业和岗位做笛卡尔积批量计算，单次最多 `300` 组
  - 返回：`data`（成功结果）+ `failed`（失败结果）+ `summary`
- `GET /match/major-job/results`
  - 查询参数：`page`、`page_size`、`sort_by`、`sort_order`
  - 筛选参数：`major_id`、`job_id`、`algorithm_version`、`run_mode(single|batch)`
  - 返回：主流程历史匹配记录及解释字段
- `DELETE /match/major-job/results/{id}`
  - 说明：管理员接口（需要 `admin`）
  - 行为：删除单条主流程历史匹配记录

### 学生-岗位匹配（扩展能力，已实现）

- `POST /match/single`
  - 请求体（必填）：`student_id`、`job_id`
  - 请求体（可选）：`algorithm_version`（默认 `v0.1-baseline`）、`persist`（默认 `true`）
  - 请求体（可选地点约束）：`preferred_location_province`、`preferred_location_city`
  - 返回：`semantic_score`、`skill_score`、`constraint_score`、`final_score`、解释字段（`matched_skills` / `missing_skills` / `constraint_failures`）
- `POST /match/batch`
  - 请求体（必填）：`student_ids`、`job_ids`（数组）
  - 请求体（可选）：`algorithm_version`、`persist`
  - 请求体（可选地点约束）：`preferred_location_province`、`preferred_location_city`
  - 说明：按学生和岗位做笛卡尔积批量计算，单次最多 `200` 组
  - 返回：`data`（成功结果）+ `failed`（失败结果）+ `summary`
- `GET /match-results`
  - 查询参数：`page`、`page_size`、`sort_by`、`sort_order`
  - 筛选参数：`student_id`、`major_id`、`job_id`、`algorithm_version`
  - 返回：历史持久化匹配记录及解释字段

### 就业重算（扩展能力，已实现）

- `POST /match/major/{id}/recompute`
  - 说明：按专业批量重算“学生-最近就业岗位”匹配
  - 请求体可选：`algorithm_version`、`persist`、`max_pairs`
- `POST /match/college/{id}/recompute`
  - 说明：管理员接口；按学院批量重算
  - 请求体可选：`algorithm_version`、`persist`、`max_pairs`
- `POST /match/recompute-all`
  - 说明：管理员接口；全量批量重算
  - 请求体可选：`algorithm_version`、`persist`、`max_pairs`

## 专业决策看板（已实现）

- `GET /dashboard/major-alignment`
  - 说明：需要登录态；按专业聚合就业与对口情况
  - 查询参数（可选）：
    - `algorithm_version`：匹配算法版本
    - `alignment_threshold`：判定“对口”的分数阈值（默认 `70`，范围 `0-100`）
    - `min_students`：最少学生样本数（默认 `1`）
  - 返回字段（`data[]`）：
    - `major_id`、`major_code`、`major_name`、`college_name`
    - `total_students`、`employed_students`
    - `employed_with_match_students`（有匹配结果的就业人数）
    - `aligned_students`（`final_score >= alignment_threshold`）
    - `avg_match_score`、`weighted_avg_match_score`
    - `excellent_graduate_score`（专业优秀毕业分）
    - `rank_position`（按优秀毕业分降序排名）
    - `employment_rate`、`alignment_rate`

- `GET /dashboard/major-skill-gaps`
  - 说明：需要登录态；按专业统计已就业学生的技能缺口 TopN
  - 查询参数（可选）：
    - `algorithm_version`：匹配算法版本
    - `top_n`：每个专业返回缺口 TopN（默认 `5`，范围 `1-20`）
    - `min_gap_count`：技能缺口最小出现次数（默认 `1`）
  - 返回结构：
    - `data[]`：按专业分组
    - 每个专业包含 `skills[]`，元素字段：
      - `rank`、`skill_name`
      - `gap_count`（缺口出现次数）
      - `student_count`（涉及学生人数）
      - `mandatory_count`（必备技能缺口次数）

## 决策看板 AI 解读（v0.6，已实现）

- `GET /dashboard/insight/profiles`
  - 说明：需要登录态；按当前系统预路由策略，建议管理员角色调用
  - 行为：返回可用的 AI Agent 配置列表与默认配置
  - 返回：`data[]`（`id/display_name/provider/model/rag_enabled/endpoint_preview/api_key_env/is_default`）+ `meta.default_profile`

- `POST /dashboard/insight`
  - 说明：需要登录态；按当前系统预路由策略，建议管理员角色调用
  - 行为：读取决策看板数据快照并调用受约束的 Agent 模型生成结构化解读
  - 请求体（可选）：
    - 范围参数：`scope(all|college|major)`、`college_id`、`major_id`
    - 看板参数：`algorithm_version`、`alignment_threshold`、`min_students`、`smoothing_k`、`top_n`、`min_gap_count`
    - Agent 参数：`profile_id`、`focus`（`balanced|alignment|gap|risk|employment`）、`tone`（`neutral|strict|encourage|executive`）、`custom_prompt`
  - 返回：`data`
    - `status(success|failed)`、`run_id`（持久化成功时）
    - `scope/scope_label/target_college_id/target_major_id`
    - `profile_id/provider/model/latency_ms`
    - `snapshot`（看板快照）
    - `insight`（结构化解读）
    - `retrieved_context`（RAG 召回片段）
    - `error`（失败时）

- `GET /dashboard/insight/runs`
  - 说明：需要登录态；按当前系统预路由策略，建议管理员角色调用
  - 行为：查询 AI 解读历史。若后续放开非管理员权限，接口已支持普通用户仅看本人记录。
  - 查询参数：
    - 分页：`page`、`page_size`、`sort_order`
    - 过滤：`scope`、`status`、`profile_id`、`focus`、`keyword`
    - `include_payload=true|false`（默认 `true`，控制是否返回完整 `snapshot/insight/retrieved_context`）
  - 返回：`data[]` + 分页字段（`count/page/page_size/total_pages`）

## 系统日志（已实现）

- `GET /system-logs`
  - 说明：需要管理员权限
  - 查询参数：`page`、`page_size`、`sort_by`、`sort_order`
  - 筛选参数：`actor_user_id`、`target_id`、`target_type`、`action`、`keyword`
