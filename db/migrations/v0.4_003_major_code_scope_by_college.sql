-- v0.4 allow duplicate major code across different colleges

USE graduate_match;
SET NAMES utf8mb4;

-- Drop legacy global unique index on majors.code if it exists.
SET @idx_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'majors'
    AND INDEX_NAME = 'code'
);
SET @sql := IF(@idx_exists > 0, 'ALTER TABLE majors DROP INDEX `code`', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Add scoped unique index: code only needs to be unique within a college.
SET @idx_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'majors'
    AND INDEX_NAME = 'uk_college_major_code'
);
SET @sql := IF(
  @idx_exists = 0,
  'ALTER TABLE majors ADD UNIQUE KEY uk_college_major_code (college_id, code)',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
