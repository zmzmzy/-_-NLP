# 当前完成度测试手册（v0.2）

日期：2026-03-02  
适用范围：`graduation_proj`

## 0. 准备

1. 初始化数据库（推荐：完整样本一键导入）

```bash
cd /home/roamer/graduation_proj
./db/load_full_seed.sh
```

该脚本会自动执行：

- `db/schema.sql`
- `db/seed_full.sql`
- `db/verify_full_seed.sql`

2. 如果只想使用最小样本，可手动执行：

```bash
cd /home/roamer/graduation_proj
mysql -uroot -p123456 < db/schema.sql
mysql -uroot -p123456 < db/seed_minimal.sql
```

3. 编译并启动后端

```bash
cd /home/roamer/graduation_proj/backend
cmake -S . -B build
cmake --build build -j
cd build
./graduate_match_backend
```

## 1. 基础健康检查

```bash
curl http://127.0.0.1:5555/api/health
```

期望：返回 `status=ok`。

补充（完整样本验收）：

```bash
mysql -uroot -p123456 < /home/roamer/graduation_proj/db/verify_full_seed.sql
```

关键期望：

- `colleges >= 10`
- `majors >= 20`
- `major_profiles = majors`
- `majors_with_students = majors`
- `majors_with_employment = majors`

## 2. 认证能力（P2-02）

1. 登录

```bash
TOKEN=$(curl -s -X POST http://127.0.0.1:5555/api/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"<admin_username>","password":"<admin_password>"}' \
  | sed -n 's/.*"token":"\([^"]*\)".*/\1/p')
echo "$TOKEN"
```

2. 从返回 JSON 中取 `token`，验证会话

```bash
curl -s http://127.0.0.1:5555/api/auth/me \
  -H "Authorization: Bearer $TOKEN"
```

3. 登出

```bash
curl -s -X POST http://127.0.0.1:5555/api/auth/logout \
  -H "Authorization: Bearer $TOKEN"
```

说明：后续步骤如需继续调用受保护接口，请重新登录获取新的 `TOKEN`。

## 3. 操作日志能力（P2-03）

1. 先做一个写操作（示例：新增学院）

```bash
CODE="CLOG$(date +%s)"
curl -s -X POST http://127.0.0.1:5555/api/colleges \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d "{\"code\":\"$CODE\",\"name\":\"日志验证学院\",\"description\":\"用于测试\"}"
```

2. 查询系统日志（管理员接口）

```bash
curl -s "http://127.0.0.1:5555/api/system-logs?page=1&page_size=10&sort_by=id&sort_order=desc" \
  -H "Authorization: Bearer $TOKEN"
```

期望：能看到刚才的 `POST /api/colleges` 日志记录。

如果返回 `duplicate college code or name`，说明 `code` 已存在，不是系统故障，换个 `code` 重新提交即可。

## 4. 匹配引擎能力（P3）

先确保 `TOKEN` 有效（如前面已执行过登出，请先重新登录）：

```bash
TOKEN=$(curl -s -X POST http://127.0.0.1:5555/api/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"<admin_username>","password":"<admin_password>"}' \
  | sed -n 's/.*"token":"\([^"]*\)".*/\1/p')
```

主流程单条计算（专业-岗位）：

```bash
curl -s -X POST http://127.0.0.1:5555/api/match/major-job \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"major_id":1001,"job_id":4001,"algorithm_version":"v0.2-major-job"}'
```

主流程批量计算（专业集 × 岗位集）：

```bash
curl -s -X POST http://127.0.0.1:5555/api/match/major-job/batch \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"major_ids":[1001],"job_ids":[4001,4002],"algorithm_version":"v0.2-major-job"}'
```

主流程历史查询（需先 `persist=true`）：

```bash
curl -s -X POST http://127.0.0.1:5555/api/match/major-job \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"major_id":1001,"job_id":4001,"algorithm_version":"v0.2-major-job","persist":true}'

curl -s "http://127.0.0.1:5555/api/match/major-job/results?page=1&page_size=10&major_id=1001" \
  -H "Authorization: Bearer $TOKEN"
```

扩展能力：学生-岗位单条计算：

```bash
curl -s -X POST http://127.0.0.1:5555/api/match/single \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"student_id":5001,"job_id":4001,"persist":true}'
```

扩展能力：学生-岗位批量计算：

```bash
curl -s -X POST http://127.0.0.1:5555/api/match/batch \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"student_ids":[5001],"job_ids":[4001],"persist":false}'
```

查看历史结果：

```bash
curl -s "http://127.0.0.1:5555/api/match-results?page=1&page_size=10&student_id=5001" \
  -H "Authorization: Bearer $TOKEN"
```

## 5. 一次性冒烟脚本（可选）

你可以按以下顺序快速跑一遍：

1. `health`
2. `auth/login`
3. `auth/me`
4. `POST /colleges`（写入）
5. `GET /system-logs`
6. `POST /match/single`
7. `GET /match-results`
8. `auth/logout`

## 6. 前端登录守卫验证（P4-01）

1. 启动前端

```bash
cd /home/roamer/graduation_proj/frontend
npm run dev
```

2. 打开 `http://localhost:5173`

- 未登录时应自动跳转到 `/login`
- 用你自己的管理员账号登录后进入首页
- 刷新页面后保持登录态（token 已持久化）
- 点击右上角“退出登录”后返回 `/login`

## 7. 前端数据管理页验证（P4-02）

登录后依次进入以下页面并验证：

1. `/majors`
- 新增学院，学院列表可见
- 新增专业，专业列表可见
- 选择专业并保存专业画像（成功提示）

2. `/jobs`
- 新增企业，企业列表可见
- 新增岗位并关联企业，岗位列表可见

3. `/students`
- 新增学生，学生列表可见
- 新增就业记录（学生+企业+可选岗位），就业记录列表可见

补充验证：

- 每个列表项都可点击“编辑”“删除”
- 删除成功后对应列表刷新

## 8. 前端匹配分析页验证（P4-03）

进入 `/match` 页面，验证以下流程：

1. 单条匹配
- 选择学生和岗位，点击“执行单条匹配”
- 页面展示 `semantic_score / skill_score / constraint_score / final_score`
- 页面展示命中技能、缺失技能、约束失败项

2. 批量匹配
- 输入学生ID列表和岗位ID列表（逗号分隔）
- 点击“执行批量匹配”
- 页面展示 summary（总组合/成功/失败）和成功/失败列表

3. 历史结果
- 点击“查询历史结果”
- 列表可分页
- 点击“查看”可展开解释详情

## 9. 专业决策看板验证（P4-04）

后端接口快速验证：

```bash
TOKEN=$(curl -s -X POST http://127.0.0.1:5555/api/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"<admin_username>","password":"<admin_password>"}' \
  | sed -n 's/.*"token":"\([^"]*\)".*/\1/p')

curl -s "http://127.0.0.1:5555/api/dashboard/major-alignment?alignment_threshold=70&min_students=1" \
  -H "Authorization: Bearer $TOKEN"

curl -s "http://127.0.0.1:5555/api/dashboard/major-skill-gaps?top_n=5&min_gap_count=1" \
  -H "Authorization: Bearer $TOKEN"
```

期望：

- `/dashboard/major-alignment` 返回 `data[]`，包含 `employment_rate`、`alignment_rate`、`excellent_graduate_score`、`rank_position`
- `/dashboard/major-skill-gaps` 返回按专业分组的数据结构（`data[].skills[]`）

前端页面验证（`/`）：

1. 打开“决策看板”页面，初次进入自动加载数据
2. 修改筛选条件后点击“刷新看板”，卡片与表格同步变化
3. “专业对口率明细”表格可看到专业、就业率、对口率
4. “专业技能缺口”区域按专业展示 TopN 缺口技能

## 10. 数据库迁移验证（v0.5）

如果你是在历史库基础上升级，而不是重新执行 `db/schema.sql`，先执行：

```bash
cd /home/roamer/graduation_proj
mysql -uroot -p123456 graduate_match < db/migrations/v0.2_001_schema_upgrade.sql
mysql -uroot -p123456 graduate_match < db/migrations/v0.3_001_core_match_results.sql
mysql -uroot -p123456 graduate_match < db/migrations/v0.3_002_job_import_tables.sql
mysql -uroot -p123456 graduate_match < db/migrations/v0.3_003_simplify_minimal_fields.sql
mysql -uroot -p123456 graduate_match < db/migrations/v0.4_001_major_excellence_dashboard.sql
mysql -uroot -p123456 graduate_match < db/migrations/v0.4_002_remove_major_education_level.sql
mysql -uroot -p123456 graduate_match < db/migrations/v0.4_003_major_code_scope_by_college.sql
mysql -uroot -p123456 graduate_match < db/migrations/v0.4_004_allow_duplicate_major_code.sql
mysql -uroot -p123456 graduate_match < db/migrations/v0.5_001_auth_profile_review.sql
mysql -uroot -p123456 graduate_match < db/migrations/v0.5_002_employment_submission_candidates.sql
```

可选回滚（谨慎）：

```bash
cd /home/roamer/graduation_proj
mysql -uroot -p123456 graduate_match < db/migrations/v0.2_001_schema_rollback.sql
```

## 11. 岗位同步能力说明（v0.3）

岗位同步脚本与 `/api/job-sync/*` 接口已下线，项目仅保留“岗位管理 CRUD + 岗位 CSV 导入”两条数据入口。

## 12. 专业导入接口验证（v0.2）

1. 预检导入：

```bash
TOKEN=$(curl -s -X POST http://127.0.0.1:5555/api/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"<admin_username>","password":"<admin_password>"}' \
  | python3 -c 'import sys,json; print(json.load(sys.stdin).get("data",{}).get("token",""))')

CSV_TEXT='university_code,university_name,college_code,college_name,major_code,major_name,major_description,core_courses_text,knowledge_skills_text,technical_platform_text,capability_text
U001,示例大学,C901,示例学院,M901,智能制造工程,导入测试专业,课程A;课程B,技能A;技能B,平台A;平台B,能力A;能力B'

curl -s -X POST http://127.0.0.1:5555/api/majors/import/preview \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d "{\"source_type\":\"csv\",\"source_filename\":\"majors_test.csv\",\"csv_text\":\"$CSV_TEXT\"}"
```

2. 从预检返回中取 `batch_id`，执行应用：

```bash
BATCH_ID=1  # 替换为实际返回的 batch_id
curl -s -X POST http://127.0.0.1:5555/api/majors/import/apply \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d "{\"batch_id\":$BATCH_ID}"
```

说明：若该批次 `valid_rows=0`，接口会返回 `409`，不会执行应用。

3. 查询导入批次历史：

```bash
curl -s "http://127.0.0.1:5555/api/majors/import/batches?page=1&page_size=10&sort_by=id&sort_order=desc" \
  -H "Authorization: Bearer $TOKEN"
```

## 13. 岗位导入接口验证（v0.3）

1. 预检导入：

```bash
TOKEN=$(curl -s -X POST http://127.0.0.1:5555/api/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"<admin_username>","password":"<admin_password>"}' \
  | python3 -c 'import sys,json; print(json.load(sys.stdin).get("data",{}).get("token",""))')

CSV_TEXT='company_name,title,job_type,min_degree,responsibilities_text,technical_projects_text,required_knowledge_skills_text,required_tools_platform_text
示例企业A,C++后端工程师,full_time,bachelor,负责后端接口开发,参与服务重构,掌握C++与MySQL,熟悉Linux与Git'

curl -s -X POST http://127.0.0.1:5555/api/jobs/import/preview \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d "{\"source_type\":\"csv\",\"source_filename\":\"jobs_test.csv\",\"csv_text\":\"$CSV_TEXT\"}"
```

2. 从预检返回中取 `batch_id`，执行应用：

```bash
BATCH_ID=1  # 替换为实际返回的 batch_id
curl -s -X POST http://127.0.0.1:5555/api/jobs/import/apply \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d "{\"batch_id\":$BATCH_ID}"
```

说明：若该批次 `valid_rows=0`，接口会返回 `409`，不会执行应用。

3. 查询导入批次历史：

```bash
curl -s "http://127.0.0.1:5555/api/jobs/import/batches?page=1&page_size=10&sort_by=id&sort_order=desc" \
  -H "Authorization: Bearer $TOKEN"
```
