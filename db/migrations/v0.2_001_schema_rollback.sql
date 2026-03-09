-- v0.2 schema rollback (best-effort)
-- Date: 2026-03-04
-- WARNING:
-- 1) This script will drop v0.2 tables and data.
-- 2) Run only when you are sure no v0.2 data needs to be preserved.

USE graduate_match;
SET NAMES utf8mb4;

SET FOREIGN_KEY_CHECKS = 0;

DROP TABLE IF EXISTS major_import_rows;
DROP TABLE IF EXISTS major_import_batches;
DROP TABLE IF EXISTS job_raw_snapshots;
DROP TABLE IF EXISTS job_sync_runs;

SET @uk_jobs_source_external_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND INDEX_NAME = 'uk_jobs_source_external'
);
SET @sql_drop_jobs_uk := IF(
  @uk_jobs_source_external_exists > 0,
  'ALTER TABLE jobs DROP INDEX uk_jobs_source_external',
  'SELECT 1'
);
PREPARE stmt_drop_jobs_uk FROM @sql_drop_jobs_uk;
EXECUTE stmt_drop_jobs_uk;
DEALLOCATE PREPARE stmt_drop_jobs_uk;

SET @idx_jobs_source_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND INDEX_NAME = 'idx_jobs_source'
);
SET @sql_drop_idx_jobs_source := IF(
  @idx_jobs_source_exists > 0,
  'ALTER TABLE jobs DROP INDEX idx_jobs_source',
  'SELECT 1'
);
PREPARE stmt_drop_idx_jobs_source FROM @sql_drop_idx_jobs_source;
EXECUTE stmt_drop_idx_jobs_source;
DEALLOCATE PREPARE stmt_drop_idx_jobs_source;

SET @idx_jobs_last_seen_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND INDEX_NAME = 'idx_jobs_last_seen'
);
SET @sql_drop_idx_jobs_last_seen := IF(
  @idx_jobs_last_seen_exists > 0,
  'ALTER TABLE jobs DROP INDEX idx_jobs_last_seen',
  'SELECT 1'
);
PREPARE stmt_drop_idx_jobs_last_seen FROM @sql_drop_idx_jobs_last_seen;
EXECUTE stmt_drop_idx_jobs_last_seen;
DEALLOCATE PREPARE stmt_drop_idx_jobs_last_seen;

SET @col_jobs_sync_batch_id_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'sync_batch_id'
);
SET @sql_drop_jobs_sync_batch_id := IF(
  @col_jobs_sync_batch_id_exists > 0,
  'ALTER TABLE jobs DROP COLUMN sync_batch_id',
  'SELECT 1'
);
PREPARE stmt_drop_jobs_sync_batch_id FROM @sql_drop_jobs_sync_batch_id;
EXECUTE stmt_drop_jobs_sync_batch_id;
DEALLOCATE PREPARE stmt_drop_jobs_sync_batch_id;

SET @col_jobs_is_active_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'is_active'
);
SET @sql_drop_jobs_is_active := IF(
  @col_jobs_is_active_exists > 0,
  'ALTER TABLE jobs DROP COLUMN is_active',
  'SELECT 1'
);
PREPARE stmt_drop_jobs_is_active FROM @sql_drop_jobs_is_active;
EXECUTE stmt_drop_jobs_is_active;
DEALLOCATE PREPARE stmt_drop_jobs_is_active;

SET @col_jobs_last_seen_at_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'last_seen_at'
);
SET @sql_drop_jobs_last_seen_at := IF(
  @col_jobs_last_seen_at_exists > 0,
  'ALTER TABLE jobs DROP COLUMN last_seen_at',
  'SELECT 1'
);
PREPARE stmt_drop_jobs_last_seen_at FROM @sql_drop_jobs_last_seen_at;
EXECUTE stmt_drop_jobs_last_seen_at;
DEALLOCATE PREPARE stmt_drop_jobs_last_seen_at;

SET @col_jobs_first_seen_at_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'first_seen_at'
);
SET @sql_drop_jobs_first_seen_at := IF(
  @col_jobs_first_seen_at_exists > 0,
  'ALTER TABLE jobs DROP COLUMN first_seen_at',
  'SELECT 1'
);
PREPARE stmt_drop_jobs_first_seen_at FROM @sql_drop_jobs_first_seen_at;
EXECUTE stmt_drop_jobs_first_seen_at;
DEALLOCATE PREPARE stmt_drop_jobs_first_seen_at;

SET @col_jobs_content_hash_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'content_hash'
);
SET @sql_drop_jobs_content_hash := IF(
  @col_jobs_content_hash_exists > 0,
  'ALTER TABLE jobs DROP COLUMN content_hash',
  'SELECT 1'
);
PREPARE stmt_drop_jobs_content_hash FROM @sql_drop_jobs_content_hash;
EXECUTE stmt_drop_jobs_content_hash;
DEALLOCATE PREPARE stmt_drop_jobs_content_hash;

SET @col_jobs_source_url_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'source_url'
);
SET @sql_drop_jobs_source_url := IF(
  @col_jobs_source_url_exists > 0,
  'ALTER TABLE jobs DROP COLUMN source_url',
  'SELECT 1'
);
PREPARE stmt_drop_jobs_source_url FROM @sql_drop_jobs_source_url;
EXECUTE stmt_drop_jobs_source_url;
DEALLOCATE PREPARE stmt_drop_jobs_source_url;

SET @col_jobs_source_job_id_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'source_job_id'
);
SET @sql_drop_jobs_source_job_id := IF(
  @col_jobs_source_job_id_exists > 0,
  'ALTER TABLE jobs DROP COLUMN source_job_id',
  'SELECT 1'
);
PREPARE stmt_drop_jobs_source_job_id FROM @sql_drop_jobs_source_job_id;
EXECUTE stmt_drop_jobs_source_job_id;
DEALLOCATE PREPARE stmt_drop_jobs_source_job_id;

SET @col_jobs_source_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'source'
);
SET @sql_drop_jobs_source := IF(
  @col_jobs_source_exists > 0,
  'ALTER TABLE jobs DROP COLUMN source',
  'SELECT 1'
);
PREPARE stmt_drop_jobs_source FROM @sql_drop_jobs_source;
EXECUTE stmt_drop_jobs_source;
DEALLOCATE PREPARE stmt_drop_jobs_source;

SET @fk_college_univ_exists := (
  SELECT COUNT(*)
  FROM information_schema.KEY_COLUMN_USAGE
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'colleges'
    AND CONSTRAINT_NAME = 'fk_colleges_university'
);
SET @sql_drop_fk_college_univ := IF(
  @fk_college_univ_exists > 0,
  'ALTER TABLE colleges DROP FOREIGN KEY fk_colleges_university',
  'SELECT 1'
);
PREPARE stmt_drop_fk_college_univ FROM @sql_drop_fk_college_univ;
EXECUTE stmt_drop_fk_college_univ;
DEALLOCATE PREPARE stmt_drop_fk_college_univ;

SET @idx_colleges_university_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'colleges'
    AND INDEX_NAME = 'idx_colleges_university'
);
SET @sql_drop_idx_colleges_university := IF(
  @idx_colleges_university_exists > 0,
  'ALTER TABLE colleges DROP INDEX idx_colleges_university',
  'SELECT 1'
);
PREPARE stmt_drop_idx_colleges_university FROM @sql_drop_idx_colleges_university;
EXECUTE stmt_drop_idx_colleges_university;
DEALLOCATE PREPARE stmt_drop_idx_colleges_university;

SET @col_colleges_university_id_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'colleges'
    AND COLUMN_NAME = 'university_id'
);
SET @sql_drop_colleges_university_id := IF(
  @col_colleges_university_id_exists > 0,
  'ALTER TABLE colleges DROP COLUMN university_id',
  'SELECT 1'
);
PREPARE stmt_drop_colleges_university_id FROM @sql_drop_colleges_university_id;
EXECUTE stmt_drop_colleges_university_id;
DEALLOCATE PREPARE stmt_drop_colleges_university_id;

DROP TABLE IF EXISTS universities;

SET FOREIGN_KEY_CHECKS = 1;
