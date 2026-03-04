USE graduate_match;

SELECT '=== 基础数量检查 ===' AS section_title;
SELECT 'colleges' AS table_name, COUNT(*) AS total FROM colleges
UNION ALL SELECT 'majors', COUNT(*) FROM majors
UNION ALL SELECT 'major_profiles', COUNT(*) FROM major_profiles
UNION ALL SELECT 'companies', COUNT(*) FROM companies
UNION ALL SELECT 'jobs', COUNT(*) FROM jobs
UNION ALL SELECT 'students', COUNT(*) FROM students
UNION ALL SELECT 'employment_records', COUNT(*) FROM employment_records
UNION ALL SELECT 'skills', COUNT(*) FROM skills
UNION ALL SELECT 'major_skills', COUNT(*) FROM major_skills
UNION ALL SELECT 'job_skills', COUNT(*) FROM job_skills
UNION ALL SELECT 'match_results', COUNT(*) FROM match_results;

SELECT '=== 覆盖率检查（导师需求关键点） ===' AS section_title;
SELECT
  (SELECT COUNT(*) FROM colleges) AS colleges_total,
  (SELECT COUNT(DISTINCT college_id) FROM majors) AS colleges_with_majors,
  (SELECT COUNT(*) FROM majors) AS majors_total,
  (SELECT COUNT(*) FROM major_profiles) AS major_profiles_total,
  (SELECT COUNT(DISTINCT major_id) FROM students) AS majors_with_students,
  (SELECT COUNT(DISTINCT s.major_id)
   FROM employment_records er
   JOIN students s ON s.id = er.student_id) AS majors_with_employment,
  (SELECT COUNT(DISTINCT major_id) FROM match_results) AS majors_with_match_results;

SELECT '=== 专业缺失检查（应返回 0 行） ===' AS section_title;
SELECT m.code, m.name, 'missing_major_profile' AS issue
FROM majors m
LEFT JOIN major_profiles mp ON mp.major_id = m.id
WHERE mp.id IS NULL
UNION ALL
SELECT m.code, m.name, 'missing_student_sample' AS issue
FROM majors m
LEFT JOIN students s ON s.major_id = m.id
WHERE s.id IS NULL
UNION ALL
SELECT m.code, m.name, 'missing_employment_sample' AS issue
FROM majors m
LEFT JOIN students s ON s.major_id = m.id
LEFT JOIN employment_records er ON er.student_id = s.id
WHERE er.id IS NULL
UNION ALL
SELECT m.code, m.name, 'missing_match_result_sample' AS issue
FROM majors m
LEFT JOIN match_results mr ON mr.major_id = m.id
WHERE mr.id IS NULL;

SELECT '=== 学院-专业分布 ===' AS section_title;
SELECT c.code AS college_code, c.name AS college_name, COUNT(m.id) AS major_count
FROM colleges c
LEFT JOIN majors m ON m.college_id = c.id
GROUP BY c.id, c.code, c.name
ORDER BY c.code;

SELECT '=== 看板核心指标预览（按专业） ===' AS section_title;
SELECT
  m.code AS major_code,
  m.name AS major_name,
  COUNT(DISTINCT s.id) AS total_students,
  COUNT(DISTINCT er.student_id) AS employed_students,
  COUNT(DISTINCT CASE WHEN mr.final_score >= 70 THEN mr.student_id END) AS aligned_students,
  ROUND(AVG(mr.final_score), 2) AS avg_final_score
FROM majors m
LEFT JOIN students s ON s.major_id = m.id
LEFT JOIN employment_records er ON er.student_id = s.id
LEFT JOIN match_results mr ON mr.student_id = s.id AND mr.job_id = er.job_id
GROUP BY m.id, m.code, m.name
ORDER BY m.code;
