USE graduate_match;
SET NAMES utf8mb4;

-- =========================================================
-- 清理答辩展示数据包 v1
-- 删除范围：
--   学校 DU_SHOW
--   学院 DC*
--   专业（学院 DC* 下）
--   学生 D26S*
--   企业/岗位（名称含答辩标记）
--   账号 def_admin / def_stu*
-- =========================================================

-- 先解除账号绑定
UPDATE users
SET linked_student_id = NULL
WHERE username = 'def_admin'
   OR username LIKE 'def_stu%';

-- 删除提交记录（按提交人与学生双路径兜底）
DELETE es
FROM employment_submissions es
JOIN users u ON u.id = es.submitter_user_id
WHERE u.username = 'def_admin' OR u.username LIKE 'def_stu%';

DELETE es
FROM employment_submissions es
JOIN students s ON s.id = es.student_id
WHERE s.student_no LIKE 'D26S%';

-- 删除匹配结果
DELETE mr
FROM match_results mr
JOIN students s ON s.id = mr.student_id
WHERE s.student_no LIKE 'D26S%';

-- 删除就业记录
DELETE er
FROM employment_records er
JOIN students s ON s.id = er.student_id
WHERE s.student_no LIKE 'D26S%';

-- 删除学生
DELETE FROM students
WHERE student_no LIKE 'D26S%';

-- 删除岗位与企业
DELETE FROM jobs
WHERE title LIKE '%（答辩J%）';

DELETE FROM companies
WHERE name LIKE '%（答辩）';

-- 删除专业画像 / 专业 / 学院 / 学校
DELETE mp
FROM major_profiles mp
JOIN majors m ON m.id = mp.major_id
JOIN colleges c ON c.id = m.college_id
WHERE c.code LIKE 'DC%';

DELETE m
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code LIKE 'DC%';

DELETE FROM colleges
WHERE code LIKE 'DC%';

DELETE FROM universities
WHERE code = 'DU_SHOW';

-- 删除答辩账号
DELETE FROM users
WHERE username = 'def_admin'
   OR username LIKE 'def_stu%';

SELECT 'defense_showcase_seed_v1_cleaned' AS message;
