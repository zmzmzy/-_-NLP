-- v0.6 add decision board AI insight run table

USE graduate_match;
SET NAMES utf8mb4;

CREATE TABLE IF NOT EXISTS dashboard_insight_runs (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  request_scope ENUM('all', 'college', 'major') NOT NULL DEFAULT 'all',
  target_college_id BIGINT NULL,
  target_major_id BIGINT NULL,
  algorithm_version VARCHAR(64) NULL,
  profile_id VARCHAR(64) NOT NULL,
  provider VARCHAR(64) NULL,
  model VARCHAR(128) NULL,
  focus VARCHAR(32) NOT NULL DEFAULT 'balanced',
  tone VARCHAR(32) NOT NULL DEFAULT 'neutral',
  custom_prompt TEXT NULL,
  status ENUM('success', 'failed') NOT NULL DEFAULT 'failed',
  latency_ms INT NOT NULL DEFAULT 0,
  snapshot_json JSON NOT NULL,
  retrieved_context_json JSON NULL,
  insight_json JSON NULL,
  prompt_text MEDIUMTEXT NULL,
  raw_response_text MEDIUMTEXT NULL,
  error_message TEXT NULL,
  created_by_user_id BIGINT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_insight_runs_scope (request_scope),
  INDEX idx_insight_runs_status (status),
  INDEX idx_insight_runs_profile (profile_id),
  INDEX idx_insight_runs_created (created_at),
  CONSTRAINT fk_insight_runs_college
    FOREIGN KEY (target_college_id) REFERENCES colleges(id)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT fk_insight_runs_major
    FOREIGN KEY (target_major_id) REFERENCES majors(id)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT fk_insight_runs_user
    FOREIGN KEY (created_by_user_id) REFERENCES users(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB;
