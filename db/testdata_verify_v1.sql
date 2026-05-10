USE graduate_match;
SET NAMES utf8mb4;

-- =========================================================
-- 备用测试数据包 v1 验收脚本
-- 目标：
--   4 学院 / 8 专业 / 16 学生 / 10 岗位 / 16 就业记录
--   账号 demo_% 可用
-- =========================================================

SELECT 'test_colleges_count' AS check_item,
       COUNT(*) AS actual_value,
       4 AS expected_min,
       IF(COUNT(*) >= 4, 'PASS', 'FAIL') AS status
FROM colleges
WHERE code LIKE 'TC%';

SELECT 'test_majors_count' AS check_item,
       COUNT(*) AS actual_value,
       8 AS expected_min,
       IF(COUNT(*) >= 8, 'PASS', 'FAIL') AS status
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code LIKE 'TC%';

SELECT 'test_students_count' AS check_item,
       COUNT(*) AS actual_value,
       16 AS expected_min,
       IF(COUNT(*) >= 16, 'PASS', 'FAIL') AS status
FROM students
WHERE student_no LIKE '2026T%';

SELECT 'test_companies_count' AS check_item,
       COUNT(*) AS actual_value,
       6 AS expected_min,
       IF(COUNT(*) >= 6, 'PASS', 'FAIL') AS status
FROM companies
WHERE name LIKE '%（测试）';

SELECT 'test_jobs_count' AS check_item,
       COUNT(*) AS actual_value,
       10 AS expected_min,
       IF(COUNT(*) >= 10, 'PASS', 'FAIL') AS status
FROM jobs
WHERE title LIKE '%（测试）';

SELECT 'test_employments_count' AS check_item,
       COUNT(*) AS actual_value,
       16 AS expected_min,
       IF(COUNT(*) >= 16, 'PASS', 'FAIL') AS status
FROM employment_records er
JOIN students s ON s.id = er.student_id
WHERE s.student_no LIKE '2026T%';

SELECT 'test_demo_accounts_count' AS check_item,
       COUNT(*) AS actual_value,
       5 AS expected_min,
       IF(COUNT(*) >= 5, 'PASS', 'FAIL') AS status
FROM users
WHERE username = 'demo_admin' OR username LIKE 'demo_stu%';

SELECT 'test_demo_student_bindings' AS check_item,
       COUNT(*) AS bound_count,
       4 AS expected_min,
       IF(COUNT(*) >= 4, 'PASS', 'FAIL') AS status
FROM users
WHERE username LIKE 'demo_stu%' AND linked_student_id IS NOT NULL;
