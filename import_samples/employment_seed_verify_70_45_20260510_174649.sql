USE graduate_match;
SET NAMES utf8mb4;

WITH latest_emp AS (
  SELECT er.student_id, er.job_id, er.employment_type,
         ROW_NUMBER() OVER (PARTITION BY er.student_id ORDER BY er.created_at DESC, er.id DESC) rn
  FROM employment_records er
),
emp AS (
  SELECT student_id, job_id
  FROM latest_emp
  WHERE rn=1 AND employment_type <> 'pending' AND job_id IS NOT NULL
),
latest_match AS (
  SELECT mr.student_id, mr.job_id, mr.final_score,
         ROW_NUMBER() OVER (PARTITION BY mr.student_id, mr.job_id ORDER BY mr.generated_at DESC, mr.id DESC) rn
  FROM match_results mr
)
SELECT
  (SELECT COUNT(*) FROM students) AS total_students,
  (SELECT COUNT(*) FROM emp) AS employed_students,
  ROUND((SELECT COUNT(*) FROM emp)*100.0/(SELECT COUNT(*) FROM students), 2) AS employment_rate_pct,
  (SELECT COUNT(*) FROM emp e
    LEFT JOIN latest_match lm ON lm.student_id=e.student_id AND lm.job_id=e.job_id AND lm.rn=1
    WHERE lm.final_score >= 70) AS aligned_students,
  ROUND((SELECT COUNT(*) FROM emp e
    LEFT JOIN latest_match lm ON lm.student_id=e.student_id AND lm.job_id=e.job_id AND lm.rn=1
    WHERE lm.final_score >= 70)*100.0/NULLIF((SELECT COUNT(*) FROM emp),0), 2) AS alignment_rate_pct;

SELECT scenario, COUNT(*) AS cnt FROM (
  SELECT 'approved' AS scenario FROM employment_submissions WHERE status='approved'
  UNION ALL
  SELECT 'pending' AS scenario FROM employment_submissions WHERE status='pending'
  UNION ALL
  SELECT 'rejected' AS scenario FROM employment_submissions WHERE status='rejected'
) t GROUP BY scenario ORDER BY scenario;
