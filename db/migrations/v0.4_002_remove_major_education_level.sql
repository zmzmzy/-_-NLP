-- v0.4 remove majors.education_level

USE graduate_match;
SET NAMES utf8mb4;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'majors'
    AND COLUMN_NAME = 'education_level'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE majors DROP COLUMN education_level', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
