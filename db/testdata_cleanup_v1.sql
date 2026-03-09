USE graduate_match;
SET NAMES utf8mb4;

-- =========================================================
-- 清理 testdata_seed_v1.sql 导入的数据
-- 仅删除“测试前缀”数据：
--   学校: TU_DEMO
--   学院: TC*
--   学生: 2026T*
--   企业/岗位: 名称含（测试）
--   账号: demo_*
-- =========================================================

-- 先解除 demo 用户与学生的绑定
UPDATE users
SET linked_student_id = NULL
WHERE username LIKE 'demo_%';

-- 删除就业提交（按提交人 / 学生两条路径兜底）
DELETE es
FROM employment_submissions es
JOIN users u ON u.id = es.submitter_user_id
WHERE u.username LIKE 'demo_%';

DELETE es
FROM employment_submissions es
JOIN students s ON s.id = es.student_id
WHERE s.student_no LIKE '2026T%';

-- 删除匹配结果
DELETE mr
FROM match_results mr
JOIN students s ON s.id = mr.student_id
WHERE s.student_no LIKE '2026T%';

-- 删除就业记录
DELETE er
FROM employment_records er
JOIN students s ON s.id = er.student_id
WHERE s.student_no LIKE '2026T%';

-- 删除学生
DELETE FROM students
WHERE student_no LIKE '2026T%';

-- 删除岗位 / 企业
DELETE FROM jobs
WHERE title LIKE '%（测试）';

DELETE FROM companies
WHERE name LIKE '%（测试）';

-- 删除专业画像 / 专业 / 学院 / 学校
DELETE mp
FROM major_profiles mp
JOIN majors m ON m.id = mp.major_id
JOIN colleges c ON c.id = m.college_id
WHERE c.code LIKE 'TC%';

DELETE m
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code LIKE 'TC%';

DELETE FROM colleges
WHERE code LIKE 'TC%';

DELETE FROM universities
WHERE code = 'TU_DEMO';

-- 删除测试账号
DELETE FROM users
WHERE username LIKE 'demo_%'
   OR email LIKE 'demo_%@test.local';

SELECT 'test_data_pack_v1_cleaned' AS message;
