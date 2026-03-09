-- v0.5 add auth/profile/review capability

USE graduate_match;
SET NAMES utf8mb4;

-- users.email
SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'users'
    AND COLUMN_NAME = 'email'
);
SET @sql := IF(@col_exists = 0, 'ALTER TABLE users ADD COLUMN email VARCHAR(128) NULL', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'users'
    AND INDEX_NAME = 'uk_users_email'
);
SET @sql := IF(@idx_exists = 0, 'ALTER TABLE users ADD UNIQUE KEY uk_users_email (email)', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- users.linked_student_id
SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'users'
    AND COLUMN_NAME = 'linked_student_id'
);
SET @sql := IF(@col_exists = 0, 'ALTER TABLE users ADD COLUMN linked_student_id BIGINT NULL', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists := (
  SELECT COUNT(*)
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'users'
    AND INDEX_NAME = 'uk_users_linked_student'
);
SET @sql := IF(@idx_exists = 0, 'ALTER TABLE users ADD UNIQUE KEY uk_users_linked_student (linked_student_id)', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

CREATE TABLE IF NOT EXISTS password_reset_tokens (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  user_id BIGINT NOT NULL,
  email VARCHAR(128) NOT NULL,
  token VARCHAR(128) NOT NULL UNIQUE,
  expires_at DATETIME NOT NULL,
  used_at DATETIME NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_password_reset_user (user_id),
  INDEX idx_password_reset_email (email),
  INDEX idx_password_reset_expires (expires_at),
  CONSTRAINT fk_password_reset_user
    FOREIGN KEY (user_id) REFERENCES users(id)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS employment_submissions (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  submitter_user_id BIGINT NOT NULL,
  student_id BIGINT NOT NULL,
  company_id BIGINT NOT NULL,
  job_id BIGINT NULL,
  employment_type ENUM('signed', 'offer', 'internship', 'pending') NOT NULL DEFAULT 'signed',
  employment_status ENUM('active', 'left', 'unknown') NOT NULL DEFAULT 'active',
  start_date DATE NULL,
  end_date DATE NULL,
  evidence_source VARCHAR(255) NULL,
  notes TEXT NULL,
  status ENUM('pending', 'approved', 'rejected') NOT NULL DEFAULT 'pending',
  review_comment TEXT NULL,
  reviewed_by_user_id BIGINT NULL,
  reviewed_at DATETIME NULL,
  approved_employment_record_id BIGINT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_emp_submission_submitter (submitter_user_id),
  INDEX idx_emp_submission_student (student_id),
  INDEX idx_emp_submission_status (status),
  INDEX idx_emp_submission_reviewed_by (reviewed_by_user_id),
  CONSTRAINT fk_emp_submission_submitter
    FOREIGN KEY (submitter_user_id) REFERENCES users(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_emp_submission_student
    FOREIGN KEY (student_id) REFERENCES students(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_emp_submission_company
    FOREIGN KEY (company_id) REFERENCES companies(id)
    ON DELETE RESTRICT ON UPDATE CASCADE,
  CONSTRAINT fk_emp_submission_job
    FOREIGN KEY (job_id) REFERENCES jobs(id)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT fk_emp_submission_reviewer
    FOREIGN KEY (reviewed_by_user_id) REFERENCES users(id)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT fk_emp_submission_employment
    FOREIGN KEY (approved_employment_record_id) REFERENCES employment_records(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB;
