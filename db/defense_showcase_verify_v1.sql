USE graduate_match;
SET NAMES utf8mb4;

-- =========================================================
-- 答辩展示数据包 v1 验收脚本
-- 目标：
--   10 学院 / 20 专业 / 100 学生 / 100 就业记录
--   专业文本 >= 150 字
--   公司 / 岗位 / 就业文本 >= 100 字
-- =========================================================

SELECT 'defense_colleges_count' AS check_item,
       COUNT(*) AS actual_value,
       10 AS expected_min,
       IF(COUNT(*) >= 10, 'PASS', 'FAIL') AS status
FROM colleges
WHERE code LIKE 'DC%';

SELECT 'defense_majors_count' AS check_item,
       COUNT(*) AS actual_value,
       20 AS expected_min,
       IF(COUNT(*) >= 20, 'PASS', 'FAIL') AS status
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code LIKE 'DC%';

SELECT 'defense_students_count' AS check_item,
       COUNT(*) AS actual_value,
       100 AS expected_min,
       IF(COUNT(*) >= 100, 'PASS', 'FAIL') AS status
FROM students
WHERE student_no LIKE 'D26S%';

SELECT 'defense_employment_count' AS check_item,
       COUNT(*) AS actual_value,
       100 AS expected_min,
       IF(COUNT(*) >= 100, 'PASS', 'FAIL') AS status
FROM employment_records er
JOIN students s ON s.id = er.student_id
WHERE s.student_no LIKE 'D26S%';

SELECT 'defense_major_text_len_min' AS check_item,
       MIN(CHAR_LENGTH(m.description)) AS actual_min_len,
       150 AS expected_min,
       IF(MIN(CHAR_LENGTH(m.description)) >= 150, 'PASS', 'FAIL') AS status
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code LIKE 'DC%';

SELECT 'defense_major_profile_len_min' AS check_item,
       MIN(CHAR_LENGTH(mp.core_courses_text)) AS min_core_courses_len,
       MIN(CHAR_LENGTH(mp.knowledge_skills_text)) AS min_knowledge_skills_len,
       MIN(CHAR_LENGTH(mp.technical_platform_text)) AS min_technical_platform_len,
       MIN(CHAR_LENGTH(mp.capability_text)) AS min_capability_len,
       IF(
         MIN(CHAR_LENGTH(mp.core_courses_text)) >= 150
         AND MIN(CHAR_LENGTH(mp.knowledge_skills_text)) >= 150
         AND MIN(CHAR_LENGTH(mp.technical_platform_text)) >= 150
         AND MIN(CHAR_LENGTH(mp.capability_text)) >= 150,
         'PASS',
         'FAIL'
       ) AS status
FROM major_profiles mp
JOIN majors m ON m.id = mp.major_id
JOIN colleges c ON c.id = m.college_id
WHERE c.code LIKE 'DC%';

SELECT 'defense_company_text_len_min' AS check_item,
       MIN(CHAR_LENGTH(co.main_business)) AS min_main_business_len,
       MIN(CHAR_LENGTH(co.flagship_products)) AS min_flagship_products_len,
       MIN(CHAR_LENGTH(co.description)) AS min_description_len,
       IF(
         MIN(CHAR_LENGTH(co.main_business)) >= 100
         AND MIN(CHAR_LENGTH(co.flagship_products)) >= 100
         AND MIN(CHAR_LENGTH(co.description)) >= 100,
         'PASS',
         'FAIL'
       ) AS status
FROM companies co
WHERE co.name LIKE '%（答辩）';

SELECT 'defense_job_text_len_min' AS check_item,
       MIN(CHAR_LENGTH(j.responsibilities_text)) AS min_responsibilities_len,
       MIN(CHAR_LENGTH(j.technical_projects_text)) AS min_technical_projects_len,
       MIN(CHAR_LENGTH(j.required_knowledge_skills_text)) AS min_required_knowledge_skills_len,
       MIN(CHAR_LENGTH(j.required_tools_platform_text)) AS min_required_tools_platform_len,
       IF(
         MIN(CHAR_LENGTH(j.responsibilities_text)) >= 100
         AND MIN(CHAR_LENGTH(j.technical_projects_text)) >= 100
         AND MIN(CHAR_LENGTH(j.required_knowledge_skills_text)) >= 100
         AND MIN(CHAR_LENGTH(j.required_tools_platform_text)) >= 100,
         'PASS',
         'FAIL'
       ) AS status
FROM jobs j
WHERE j.title LIKE '%（答辩J%）';

SELECT 'defense_employment_notes_len_min' AS check_item,
       MIN(CHAR_LENGTH(er.notes)) AS actual_min_len,
       100 AS expected_min,
       IF(MIN(CHAR_LENGTH(er.notes)) >= 100, 'PASS', 'FAIL') AS status
FROM employment_records er
JOIN students s ON s.id = er.student_id
WHERE s.student_no LIKE 'D26S%';

SELECT 'defense_student_text_len_min' AS check_item,
       MIN(CHAR_LENGTH(s.resume_text)) AS min_resume_len,
       MIN(CHAR_LENGTH(s.technical_projects_text)) AS min_technical_projects_len,
       MIN(CHAR_LENGTH(s.capability_summary_text)) AS min_capability_summary_len,
       IF(
         MIN(CHAR_LENGTH(s.resume_text)) >= 100
         AND MIN(CHAR_LENGTH(s.technical_projects_text)) >= 100
         AND MIN(CHAR_LENGTH(s.capability_summary_text)) >= 100,
         'PASS',
         'FAIL'
       ) AS status
FROM students s
WHERE s.student_no LIKE 'D26S%';
