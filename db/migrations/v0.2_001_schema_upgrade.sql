-- v0.2 schema upgrade
-- Date: 2026-03-04
-- Target DB: graduate_match (MySQL 8.x)

USE graduate_match;
SET NAMES utf8mb4;

CREATE TABLE IF NOT EXISTS universities (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  code VARCHAR(64) NOT NULL UNIQUE,
  name VARCHAR(255) NOT NULL UNIQUE,
  province VARCHAR(64) NULL,
  city VARCHAR(64) NULL,
  description TEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

SET @col_colleges_university_id_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'colleges'
    AND COLUMN_NAME = 'university_id'
);
SET @sql_add_colleges_university_id := IF(
  @col_colleges_university_id_exists = 0,
  'ALTER TABLE colleges ADD COLUMN university_id BIGINT NULL',
  'SELECT 1'
);
PREPARE stmt_add_colleges_university_id FROM @sql_add_colleges_university_id;
EXECUTE stmt_add_colleges_university_id;
DEALLOCATE PREPARE stmt_add_colleges_university_id;

SET @idx_colleges_university_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'colleges'
    AND INDEX_NAME = 'idx_colleges_university'
);
SET @sql_idx_colleges_university := IF(
  @idx_colleges_university_exists = 0,
  'ALTER TABLE colleges ADD INDEX idx_colleges_university (university_id)',
  'SELECT 1'
);
PREPARE stmt_idx_colleges_university FROM @sql_idx_colleges_university;
EXECUTE stmt_idx_colleges_university;
DEALLOCATE PREPARE stmt_idx_colleges_university;

SET @fk_college_univ_exists := (
  SELECT COUNT(*)
  FROM information_schema.KEY_COLUMN_USAGE
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'colleges'
    AND CONSTRAINT_NAME = 'fk_colleges_university'
);
SET @sql_college_univ_fk := IF(
  @fk_college_univ_exists = 0,
  'ALTER TABLE colleges ADD CONSTRAINT fk_colleges_university FOREIGN KEY (university_id) REFERENCES universities(id) ON DELETE SET NULL ON UPDATE CASCADE',
  'SELECT 1'
);
PREPARE stmt_college_univ_fk FROM @sql_college_univ_fk;
EXECUTE stmt_college_univ_fk;
DEALLOCATE PREPARE stmt_college_univ_fk;

SET @col_jobs_source_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'source'
);
SET @sql_add_jobs_source := IF(
  @col_jobs_source_exists = 0,
  'ALTER TABLE jobs ADD COLUMN source VARCHAR(64) NULL',
  'SELECT 1'
);
PREPARE stmt_add_jobs_source FROM @sql_add_jobs_source;
EXECUTE stmt_add_jobs_source;
DEALLOCATE PREPARE stmt_add_jobs_source;

SET @col_jobs_source_job_id_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'source_job_id'
);
SET @sql_add_jobs_source_job_id := IF(
  @col_jobs_source_job_id_exists = 0,
  'ALTER TABLE jobs ADD COLUMN source_job_id VARCHAR(128) NULL',
  'SELECT 1'
);
PREPARE stmt_add_jobs_source_job_id FROM @sql_add_jobs_source_job_id;
EXECUTE stmt_add_jobs_source_job_id;
DEALLOCATE PREPARE stmt_add_jobs_source_job_id;

SET @col_jobs_source_url_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'source_url'
);
SET @sql_add_jobs_source_url := IF(
  @col_jobs_source_url_exists = 0,
  'ALTER TABLE jobs ADD COLUMN source_url VARCHAR(512) NULL',
  'SELECT 1'
);
PREPARE stmt_add_jobs_source_url FROM @sql_add_jobs_source_url;
EXECUTE stmt_add_jobs_source_url;
DEALLOCATE PREPARE stmt_add_jobs_source_url;

SET @col_jobs_content_hash_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'content_hash'
);
SET @sql_add_jobs_content_hash := IF(
  @col_jobs_content_hash_exists = 0,
  'ALTER TABLE jobs ADD COLUMN content_hash CHAR(40) NULL',
  'SELECT 1'
);
PREPARE stmt_add_jobs_content_hash FROM @sql_add_jobs_content_hash;
EXECUTE stmt_add_jobs_content_hash;
DEALLOCATE PREPARE stmt_add_jobs_content_hash;

SET @col_jobs_first_seen_at_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'first_seen_at'
);
SET @sql_add_jobs_first_seen_at := IF(
  @col_jobs_first_seen_at_exists = 0,
  'ALTER TABLE jobs ADD COLUMN first_seen_at DATETIME NULL',
  'SELECT 1'
);
PREPARE stmt_add_jobs_first_seen_at FROM @sql_add_jobs_first_seen_at;
EXECUTE stmt_add_jobs_first_seen_at;
DEALLOCATE PREPARE stmt_add_jobs_first_seen_at;

SET @col_jobs_last_seen_at_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'last_seen_at'
);
SET @sql_add_jobs_last_seen_at := IF(
  @col_jobs_last_seen_at_exists = 0,
  'ALTER TABLE jobs ADD COLUMN last_seen_at DATETIME NULL',
  'SELECT 1'
);
PREPARE stmt_add_jobs_last_seen_at FROM @sql_add_jobs_last_seen_at;
EXECUTE stmt_add_jobs_last_seen_at;
DEALLOCATE PREPARE stmt_add_jobs_last_seen_at;

SET @col_jobs_is_active_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'is_active'
);
SET @sql_add_jobs_is_active := IF(
  @col_jobs_is_active_exists = 0,
  'ALTER TABLE jobs ADD COLUMN is_active TINYINT(1) NOT NULL DEFAULT 1',
  'SELECT 1'
);
PREPARE stmt_add_jobs_is_active FROM @sql_add_jobs_is_active;
EXECUTE stmt_add_jobs_is_active;
DEALLOCATE PREPARE stmt_add_jobs_is_active;

SET @col_jobs_sync_batch_id_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND COLUMN_NAME = 'sync_batch_id'
);
SET @sql_add_jobs_sync_batch_id := IF(
  @col_jobs_sync_batch_id_exists = 0,
  'ALTER TABLE jobs ADD COLUMN sync_batch_id BIGINT NULL',
  'SELECT 1'
);
PREPARE stmt_add_jobs_sync_batch_id FROM @sql_add_jobs_sync_batch_id;
EXECUTE stmt_add_jobs_sync_batch_id;
DEALLOCATE PREPARE stmt_add_jobs_sync_batch_id;

SET @idx_jobs_source_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND INDEX_NAME = 'idx_jobs_source'
);
SET @sql_idx_jobs_source := IF(
  @idx_jobs_source_exists = 0,
  'ALTER TABLE jobs ADD INDEX idx_jobs_source (source)',
  'SELECT 1'
);
PREPARE stmt_idx_jobs_source FROM @sql_idx_jobs_source;
EXECUTE stmt_idx_jobs_source;
DEALLOCATE PREPARE stmt_idx_jobs_source;

SET @idx_jobs_last_seen_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND INDEX_NAME = 'idx_jobs_last_seen'
);
SET @sql_idx_jobs_last_seen := IF(
  @idx_jobs_last_seen_exists = 0,
  'ALTER TABLE jobs ADD INDEX idx_jobs_last_seen (last_seen_at)',
  'SELECT 1'
);
PREPARE stmt_idx_jobs_last_seen FROM @sql_idx_jobs_last_seen;
EXECUTE stmt_idx_jobs_last_seen;
DEALLOCATE PREPARE stmt_idx_jobs_last_seen;

SET @uk_jobs_source_external_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'jobs'
    AND INDEX_NAME = 'uk_jobs_source_external'
);
SET @sql_jobs_uk := IF(
  @uk_jobs_source_external_exists = 0,
  'ALTER TABLE jobs ADD UNIQUE KEY uk_jobs_source_external (source, source_job_id)',
  'SELECT 1'
);
PREPARE stmt_jobs_uk FROM @sql_jobs_uk;
EXECUTE stmt_jobs_uk;
DEALLOCATE PREPARE stmt_jobs_uk;

CREATE TABLE IF NOT EXISTS job_sync_runs (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  source VARCHAR(64) NOT NULL,
  source_key VARCHAR(128) NOT NULL,
  status ENUM('running', 'success', 'failed') NOT NULL DEFAULT 'running',
  trigger_type ENUM('manual', 'scheduled', 'api') NOT NULL DEFAULT 'manual',
  triggered_by_user_id BIGINT NULL,
  started_at DATETIME NOT NULL,
  finished_at DATETIME NULL,
  duration_ms INT NULL,
  total_raw_jobs INT NOT NULL DEFAULT 0,
  total_normalized_jobs INT NOT NULL DEFAULT 0,
  total_upserted_jobs INT NOT NULL DEFAULT 0,
  total_new_jobs INT NOT NULL DEFAULT 0,
  total_updated_jobs INT NOT NULL DEFAULT 0,
  duplicate_count INT NOT NULL DEFAULT 0,
  failure_count INT NOT NULL DEFAULT 0,
  input_file VARCHAR(1024) NULL,
  output_file VARCHAR(1024) NULL,
  message TEXT NULL,
  error_detail TEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_job_sync_runs_source_key (source_key),
  INDEX idx_job_sync_runs_status (status),
  INDEX idx_job_sync_runs_started (started_at),
  CONSTRAINT fk_job_sync_runs_user
    FOREIGN KEY (triggered_by_user_id) REFERENCES users(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS job_raw_snapshots (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  source VARCHAR(64) NOT NULL,
  source_key VARCHAR(128) NOT NULL,
  sync_run_id BIGINT NULL,
  snapshot_at DATETIME NOT NULL,
  file_path VARCHAR(1024) NOT NULL,
  payload_json JSON NULL,
  content_hash CHAR(40) NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_job_raw_snapshots_source_key (source_key),
  INDEX idx_job_raw_snapshots_snapshot_at (snapshot_at),
  INDEX idx_job_raw_snapshots_sync_run (sync_run_id),
  CONSTRAINT fk_job_raw_snapshots_sync_run
    FOREIGN KEY (sync_run_id) REFERENCES job_sync_runs(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS major_import_batches (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  source_type ENUM('csv', 'xlsx', 'manual') NOT NULL DEFAULT 'csv',
  source_filename VARCHAR(512) NULL,
  status ENUM('preview', 'applied', 'failed') NOT NULL DEFAULT 'preview',
  triggered_by_user_id BIGINT NULL,
  total_rows INT NOT NULL DEFAULT 0,
  valid_rows INT NOT NULL DEFAULT 0,
  invalid_rows INT NOT NULL DEFAULT 0,
  inserted_rows INT NOT NULL DEFAULT 0,
  updated_rows INT NOT NULL DEFAULT 0,
  skipped_rows INT NOT NULL DEFAULT 0,
  error_rows INT NOT NULL DEFAULT 0,
  preview_payload_json JSON NULL,
  started_at DATETIME NOT NULL,
  finished_at DATETIME NULL,
  message TEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_major_import_batches_status (status),
  INDEX idx_major_import_batches_started (started_at),
  CONSTRAINT fk_major_import_batches_user
    FOREIGN KEY (triggered_by_user_id) REFERENCES users(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS major_import_rows (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  batch_id BIGINT NOT NULL,
  row_no INT NOT NULL,
  row_status ENUM('valid', 'invalid', 'inserted', 'updated', 'skipped', 'failed') NOT NULL DEFAULT 'valid',
  university_code VARCHAR(64) NULL,
  college_code VARCHAR(64) NULL,
  major_code VARCHAR(64) NULL,
  error_message TEXT NULL,
  raw_row_json JSON NULL,
  normalized_row_json JSON NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_major_import_rows_batch (batch_id),
  INDEX idx_major_import_rows_status (row_status),
  CONSTRAINT fk_major_import_rows_batch
    FOREIGN KEY (batch_id) REFERENCES major_import_batches(id)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB;
