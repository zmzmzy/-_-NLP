# 备用测试数据包操作手册（v1）

适用项目目录：`/home/roamer/graduation_proj`

数据包文件：

- 导入：`db/testdata_seed_v1.sql`
- 清理：`db/testdata_cleanup_v1.sql`

## 1. 数据内容范围

该测试包会补充以下数据（不清空现有业务数据）：

1. 学校：1 所（`TU_DEMO`）
2. 学院：4 个（`TC01`~`TC04`）
3. 专业：8 个（每学院 2 个）
4. 专业画像：8 条
5. 学生：16 名（学号前缀 `2026T`）
6. 企业：6 家（名称含 `（测试）`）
7. 岗位：10 个（名称含 `（测试）`）
8. 就业记录：16 条（每个测试学生 1 条，含部分“偏不对口”样本）
9. 账号：5 个（`demo_admin` + 4 个 `demo_stu*`）

## 2. 导入步骤

在项目根目录执行：

```bash
cd /home/roamer/graduation_proj
mysql -uroot -p123456 graduate_match < db/testdata_seed_v1.sql
```

> 如果你本机 MySQL 账号密码不同，请替换 `-uroot -p123456`。

## 3. 测试账号

所有测试账号密码统一为：`Demo@123`

1. 管理员：`demo_admin`（`admin` 角色）
2. 学生账号：
   - `demo_stu01`
   - `demo_stu02`
   - `demo_stu03`
   - `demo_stu04`

## 4. 导入后验证

执行验证 SQL：

```bash
mysql -uroot -p123456 -e "
USE graduate_match;
SELECT COUNT(*) AS colleges FROM colleges WHERE code LIKE 'TC%';
SELECT COUNT(*) AS majors FROM majors m JOIN colleges c ON c.id=m.college_id WHERE c.code LIKE 'TC%';
SELECT COUNT(*) AS students FROM students WHERE student_no LIKE '2026T%';
SELECT COUNT(*) AS companies FROM companies WHERE name LIKE '%（测试）';
SELECT COUNT(*) AS jobs FROM jobs WHERE title LIKE '%（测试）';
SELECT COUNT(*) AS employments FROM employment_records er JOIN students s ON s.id=er.student_id WHERE s.student_no LIKE '2026T%';
SELECT username, role, linked_student_id FROM users WHERE username LIKE 'demo_%' ORDER BY username;
"
```

预期大致结果：

1. 学院 `4`
2. 专业 `8`
3. 学生 `16`
4. 企业 `6`
5. 岗位 `10`
6. 就业记录 `16`

## 5. 前端演示建议路径

1. 用 `demo_admin` 登录
2. 进入“专业中心”：查看 TC 开头学院、TM 开头专业及专业画像
3. 进入“专业详情页”：查看该专业学生、就业、匹配结果
4. 进入“岗位中心”：查看测试企业与岗位
5. 进入“匹配分析”：执行批量重算（专业/学院/全量）
6. 进入“决策看板”：对比不同专业就业与匹配表现

## 6. 清理回滚

如果需要删除这批测试数据：

```bash
cd /home/roamer/graduation_proj
mysql -uroot -p123456 graduate_match < db/testdata_cleanup_v1.sql
```

清理脚本只删除测试前缀数据（`TU_DEMO` / `TC*` / `2026T*` / `demo_*` / `（测试）`）。

## 7. 注意事项

1. 本数据包是“追加导入”，不会自动清空原有数据。
2. `demo_stu*` 账号已绑定对应学生，可直接测试“学生个人中心”与“提交就业信息”。
3. 若你当前 5555 端口跑的是旧目录后端（例如 `graduation_proj_0.1`），请先停掉再启动当前项目后端，否则会出现“改了代码但页面/接口不生效”的现象。
