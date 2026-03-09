USE graduate_match;

CREATE TABLE IF NOT EXISTS job_import_batches (
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
  INDEX idx_job_import_batches_status (status),
  INDEX idx_job_import_batches_started (started_at),
  CONSTRAINT fk_job_import_batches_user
    FOREIGN KEY (triggered_by_user_id) REFERENCES users(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS job_import_rows (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  batch_id BIGINT NOT NULL,
  row_no INT NOT NULL,
  row_status ENUM('valid', 'invalid', 'inserted', 'updated', 'skipped', 'failed') NOT NULL DEFAULT 'valid',
  company_name VARCHAR(255) NULL,
  title VARCHAR(255) NULL,
  error_message TEXT NULL,
  raw_row_json JSON NULL,
  normalized_row_json JSON NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_job_import_rows_batch (batch_id),
  INDEX idx_job_import_rows_status (row_status),
  CONSTRAINT fk_job_import_rows_batch
    FOREIGN KEY (batch_id) REFERENCES job_import_batches(id)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB;
