-- v0.3 simplify schema for major-job matching core
-- Remove non-core fields for majors/jobs and retire job-sync tables

USE graduate_match;
SET NAMES utf8mb4;

-- Drop legacy job indexes that depend on removed columns.
SET @idx_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND INDEX_NAME = 'uk_jobs_source_external'
);
SET @sql := IF(@idx_exists > 0, 'ALTER TABLE jobs DROP INDEX uk_jobs_source_external', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND INDEX_NAME = 'idx_jobs_source'
);
SET @sql := IF(@idx_exists > 0, 'ALTER TABLE jobs DROP INDEX idx_jobs_source', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND INDEX_NAME = 'idx_jobs_last_seen'
);
SET @sql := IF(@idx_exists > 0, 'ALTER TABLE jobs DROP INDEX idx_jobs_last_seen', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND INDEX_NAME = 'idx_jobs_status'
);
SET @sql := IF(@idx_exists > 0, 'ALTER TABLE jobs DROP INDEX idx_jobs_status', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Drop legacy columns in majors.
SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'majors'
    AND COLUMN_NAME = 'discipline_category'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE majors DROP COLUMN discipline_category', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'majors'
    AND COLUMN_NAME = 'enrollment_year'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE majors DROP COLUMN enrollment_year', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Drop legacy columns in jobs.
SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'status'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE jobs DROP COLUMN status', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'source'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE jobs DROP COLUMN source', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'source_job_id'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE jobs DROP COLUMN source_job_id', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'source_url'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE jobs DROP COLUMN source_url', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'content_hash'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE jobs DROP COLUMN content_hash', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'first_seen_at'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE jobs DROP COLUMN first_seen_at', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'last_seen_at'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE jobs DROP COLUMN last_seen_at', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'is_active'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE jobs DROP COLUMN is_active', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'sync_batch_id'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE jobs DROP COLUMN sync_batch_id', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Drop legacy import row columns.
SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'job_import_rows'
    AND COLUMN_NAME = 'source'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE job_import_rows DROP COLUMN source', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'job_import_rows'
    AND COLUMN_NAME = 'source_job_id'
);
SET @sql := IF(@col_exists > 0, 'ALTER TABLE job_import_rows DROP COLUMN source_job_id', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Drop retired job-sync tables.
DROP TABLE IF EXISTS job_raw_snapshots;
DROP TABLE IF EXISTS job_sync_runs;
