# API 合同草案（v0.1）

基础前缀：`/api`

鉴权方式：

- 登录后获得 `token`
- 受保护接口通过请求头携带：`Authorization: Bearer <token>`

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
  - 请求体：`username`、`password`
  - 返回：`token`、`token_type`、`expires_at_epoch`、`user`
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
- `POST /jobs`
- `GET /jobs/{id}`
- `PUT /jobs/{id}`
- `DELETE /jobs/{id}`

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

## 匹配（已实现）

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

## 专业决策看板（已实现）

- `GET /dashboard/major-alignment`
  - 说明：需要登录态；按专业聚合就业与对口情况
  - 查询参数（可选）：
    - `graduation_year`：毕业年份
    - `algorithm_version`：匹配算法版本
    - `alignment_threshold`：判定“对口”的分数阈值（默认 `70`，范围 `0-100`）
    - `min_students`：最少学生样本数（默认 `1`）
  - 返回字段（`data[]`）：
    - `major_id`、`major_code`、`major_name`、`college_name`
    - `total_students`、`employed_students`
    - `employed_with_match_students`（有匹配结果的就业人数）
    - `aligned_students`（`final_score >= alignment_threshold`）
    - `avg_match_score`、`employment_rate`、`alignment_rate`

- `GET /dashboard/major-skill-gaps`
  - 说明：需要登录态；按专业统计已就业学生的技能缺口 TopN
  - 查询参数（可选）：
    - `graduation_year`：毕业年份
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

## 系统日志（已实现）

- `GET /system-logs`
  - 说明：需要管理员权限
  - 查询参数：`page`、`page_size`、`sort_by`、`sort_order`
  - 筛选参数：`actor_user_id`、`target_id`、`target_type`、`action`、`keyword`
