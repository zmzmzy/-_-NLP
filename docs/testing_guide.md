# 当前完成度测试手册（v0.1）

日期：2026-03-02  
适用范围：`graduation_proj_0.1`

## 0. 准备

1. 初始化数据库（推荐：完整样本一键导入）

```bash
cd /home/roamer/graduation_proj_0.1
./db/load_full_seed.sh
```

该脚本会自动执行：

- `db/schema.sql`
- `db/seed_full.sql`
- `db/verify_full_seed.sql`

2. 如果只想使用最小样本，可手动执行：

```bash
cd /home/roamer/graduation_proj_0.1
mysql -uroot -p123456 < db/schema.sql
mysql -uroot -p123456 < db/seed_minimal.sql
```

3. 编译并启动后端

```bash
cd /home/roamer/graduation_proj_0.1/backend
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
mysql -uroot -p123456 < /home/roamer/graduation_proj_0.1/db/verify_full_seed.sql
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
  -d '{"username":"admin","password":"admin123"}' \
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

单条计算：

```bash
curl -s -X POST http://127.0.0.1:5555/api/match/single \
  -H 'Content-Type: application/json' \
  -d '{"student_id":5001,"job_id":4001,"persist":true}'
```

批量计算：

```bash
curl -s -X POST http://127.0.0.1:5555/api/match/batch \
  -H 'Content-Type: application/json' \
  -d '{"student_ids":[5001],"job_ids":[4001],"persist":false}'
```

查看历史结果：

```bash
curl -s "http://127.0.0.1:5555/api/match-results?page=1&page_size=10&student_id=5001"
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
cd /home/roamer/graduation_proj_0.1/frontend
npm run dev
```

2. 打开 `http://localhost:5173`

- 未登录时应自动跳转到 `/login`
- 用 `admin / admin123` 登录后进入首页
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
  -d '{"username":"admin","password":"admin123"}' \
  | sed -n 's/.*"token":"\([^"]*\)".*/\1/p')

curl -s "http://127.0.0.1:5555/api/dashboard/major-alignment?alignment_threshold=70&min_students=1" \
  -H "Authorization: Bearer $TOKEN"

curl -s "http://127.0.0.1:5555/api/dashboard/major-skill-gaps?top_n=5&min_gap_count=1" \
  -H "Authorization: Bearer $TOKEN"
```

期望：

- `/dashboard/major-alignment` 返回 `data[]`，包含 `employment_rate` 与 `alignment_rate`
- `/dashboard/major-skill-gaps` 返回按专业分组的数据结构（`data[].skills[]`）

前端页面验证（`/`）：

1. 打开“决策看板”页面，初次进入自动加载数据
2. 修改筛选条件后点击“刷新看板”，卡片与表格同步变化
3. “专业对口率明细”表格可看到专业、就业率、对口率
4. “专业技能缺口”区域按专业展示 TopN 缺口技能
