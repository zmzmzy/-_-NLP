-- v0.3 draft: core major-job match result table
-- Safe to run repeatedly

USE graduate_match;

CREATE TABLE IF NOT EXISTS major_job_match_results (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  major_id BIGINT NOT NULL,
  job_id BIGINT NOT NULL,
  algorithm_version VARCHAR(64) NOT NULL,
  semantic_score DECIMAL(6,3) NOT NULL DEFAULT 0.000,
  skill_score DECIMAL(6,3) NOT NULL DEFAULT 0.000,
  constraint_score DECIMAL(6,3) NOT NULL DEFAULT 1.000,
  final_score DECIMAL(6,3) NOT NULL DEFAULT 0.000,
  matched_skills_json JSON NULL,
  missing_skills_json JSON NULL,
  constraint_failures_json JSON NULL,
  run_mode ENUM('single', 'batch') NOT NULL DEFAULT 'single',
  created_by_user_id BIGINT NULL,
  generated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_major_job_match_major (major_id),
  INDEX idx_major_job_match_job (job_id),
  INDEX idx_major_job_match_score (final_score),
  INDEX idx_major_job_match_generated (generated_at),
  CONSTRAINT fk_major_job_match_major
    FOREIGN KEY (major_id) REFERENCES majors(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_major_job_match_job
    FOREIGN KEY (job_id) REFERENCES jobs(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_major_job_match_user
    FOREIGN KEY (created_by_user_id) REFERENCES users(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB;

