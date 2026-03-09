-- v0.4 allow duplicate major code in the same college
-- Keep only college+name unique to identify majors.

USE graduate_match;
SET NAMES utf8mb4;

SET @idx_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'majors'
    AND INDEX_NAME = 'uk_college_major_code'
);
SET @sql := IF(@idx_exists > 0, 'ALTER TABLE majors DROP INDEX uk_college_major_code', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
