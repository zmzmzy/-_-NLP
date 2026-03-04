-- Schema: graduate_match
-- Target: MySQL 8.x

CREATE DATABASE IF NOT EXISTS graduate_match
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_0900_ai_ci;

USE graduate_match;

SET NAMES utf8mb4;

CREATE TABLE IF NOT EXISTS users (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  username VARCHAR(64) NOT NULL UNIQUE,
  password_hash VARCHAR(255) NOT NULL,
  role ENUM('admin', 'teacher', 'viewer') NOT NULL DEFAULT 'teacher',
  is_active TINYINT(1) NOT NULL DEFAULT 1,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS system_logs (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  actor_user_id BIGINT NULL,
  action VARCHAR(128) NOT NULL,
  target_type VARCHAR(64) NOT NULL,
  target_id BIGINT NULL,
  details_json JSON NULL,
  ip_address VARCHAR(64) NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_logs_actor (actor_user_id),
  INDEX idx_logs_target (target_type, target_id),
  INDEX idx_logs_created (created_at),
  CONSTRAINT fk_logs_user
    FOREIGN KEY (actor_user_id) REFERENCES users(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS colleges (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  code VARCHAR(32) NOT NULL UNIQUE,
  name VARCHAR(128) NOT NULL UNIQUE,
  description TEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS majors (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  college_id BIGINT NOT NULL,
  code VARCHAR(32) NOT NULL UNIQUE,
  name VARCHAR(128) NOT NULL,
  education_level ENUM('bachelor', 'master', 'phd') NOT NULL DEFAULT 'bachelor',
  discipline_category VARCHAR(128) NULL,
  enrollment_year INT NULL,
  description TEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uk_college_major (college_id, name),
  INDEX idx_majors_college (college_id),
  CONSTRAINT fk_majors_college
    FOREIGN KEY (college_id) REFERENCES colleges(id)
    ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS major_profiles (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  major_id BIGINT NOT NULL UNIQUE,
  core_courses_text TEXT NOT NULL,
  knowledge_skills_text TEXT NOT NULL,
  technical_platform_text TEXT NOT NULL,
  capability_text TEXT NOT NULL,
  updated_by_user_id BIGINT NULL,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_major_profiles_updated_by (updated_by_user_id),
  CONSTRAINT fk_major_profiles_major
    FOREIGN KEY (major_id) REFERENCES majors(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_major_profiles_user
    FOREIGN KEY (updated_by_user_id) REFERENCES users(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS companies (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(255) NOT NULL UNIQUE,
  main_business TEXT NOT NULL,
  flagship_products TEXT NOT NULL,
  domain_area TEXT NOT NULL,
  application_industries TEXT NOT NULL,
  province VARCHAR(64) NULL,
  city VARCHAR(64) NULL,
  website VARCHAR(255) NULL,
  description TEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS jobs (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  company_id BIGINT NOT NULL,
  title VARCHAR(255) NOT NULL,
  job_type ENUM('full_time', 'internship', 'contract') NOT NULL DEFAULT 'full_time',
  location_province VARCHAR(64) NULL,
  location_city VARCHAR(64) NULL,
  min_degree ENUM('none', 'diploma', 'bachelor', 'master', 'phd') NOT NULL DEFAULT 'bachelor',
  salary_range VARCHAR(64) NULL,
  responsibilities_text TEXT NOT NULL,
  technical_projects_text TEXT NOT NULL,
  required_knowledge_skills_text TEXT NOT NULL,
  required_tools_platform_text TEXT NOT NULL,
  status ENUM('open', 'closed') NOT NULL DEFAULT 'open',
  published_at DATETIME NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_jobs_company (company_id),
  INDEX idx_jobs_status (status),
  INDEX idx_jobs_degree (min_degree),
  CONSTRAINT fk_jobs_company
    FOREIGN KEY (company_id) REFERENCES companies(id)
    ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS students (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  student_no VARCHAR(64) NOT NULL UNIQUE,
  full_name VARCHAR(128) NOT NULL,
  gender ENUM('male', 'female', 'other', 'unknown') NOT NULL DEFAULT 'unknown',
  college_id BIGINT NOT NULL,
  major_id BIGINT NOT NULL,
  graduation_year INT NOT NULL,
  email VARCHAR(128) NULL,
  phone VARCHAR(32) NULL,
  resume_text LONGTEXT NULL,
  technical_projects_text TEXT NULL,
  capability_summary_text TEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_students_major (major_id),
  INDEX idx_students_college (college_id),
  INDEX idx_students_grad_year (graduation_year),
  CONSTRAINT fk_students_college
    FOREIGN KEY (college_id) REFERENCES colleges(id)
    ON DELETE RESTRICT ON UPDATE CASCADE,
  CONSTRAINT fk_students_major
    FOREIGN KEY (major_id) REFERENCES majors(id)
    ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS employment_records (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  student_id BIGINT NOT NULL,
  company_id BIGINT NOT NULL,
  job_id BIGINT NULL,
  employment_type ENUM('signed', 'offer', 'internship', 'pending') NOT NULL DEFAULT 'signed',
  employment_status ENUM('active', 'left', 'unknown') NOT NULL DEFAULT 'active',
  start_date DATE NULL,
  end_date DATE NULL,
  evidence_source VARCHAR(255) NULL,
  notes TEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_employment_student (student_id),
  INDEX idx_employment_major_job (job_id, student_id),
  CONSTRAINT fk_employment_student
    FOREIGN KEY (student_id) REFERENCES students(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_employment_company
    FOREIGN KEY (company_id) REFERENCES companies(id)
    ON DELETE RESTRICT ON UPDATE CASCADE,
  CONSTRAINT fk_employment_job
    FOREIGN KEY (job_id) REFERENCES jobs(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS skills (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(128) NOT NULL UNIQUE,
  category VARCHAR(64) NULL,
  synonyms_text TEXT NULL
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS major_skills (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  major_id BIGINT NOT NULL,
  skill_id BIGINT NOT NULL,
  weight DECIMAL(6,3) NOT NULL DEFAULT 1.000,
  source_course VARCHAR(255) NULL,
  UNIQUE KEY uk_major_skill (major_id, skill_id),
  INDEX idx_major_skills_major (major_id),
  INDEX idx_major_skills_skill (skill_id),
  CONSTRAINT fk_major_skills_major
    FOREIGN KEY (major_id) REFERENCES majors(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_major_skills_skill
    FOREIGN KEY (skill_id) REFERENCES skills(id)
    ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS job_skills (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  job_id BIGINT NOT NULL,
  skill_id BIGINT NOT NULL,
  weight DECIMAL(6,3) NOT NULL DEFAULT 1.000,
  is_mandatory TINYINT(1) NOT NULL DEFAULT 0,
  UNIQUE KEY uk_job_skill (job_id, skill_id),
  INDEX idx_job_skills_job (job_id),
  INDEX idx_job_skills_skill (skill_id),
  CONSTRAINT fk_job_skills_job
    FOREIGN KEY (job_id) REFERENCES jobs(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_job_skills_skill
    FOREIGN KEY (skill_id) REFERENCES skills(id)
    ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS match_results (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  student_id BIGINT NOT NULL,
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
  generated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_match_student (student_id),
  INDEX idx_match_major (major_id),
  INDEX idx_match_job (job_id),
  INDEX idx_match_generated (generated_at),
  CONSTRAINT fk_match_student
    FOREIGN KEY (student_id) REFERENCES students(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_match_major
    FOREIGN KEY (major_id) REFERENCES majors(id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_match_job
    FOREIGN KEY (job_id) REFERENCES jobs(id)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS major_alignment_snapshots (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  major_id BIGINT NOT NULL,
  snapshot_date DATE NOT NULL,
  total_students INT NOT NULL DEFAULT 0,
  employed_students INT NOT NULL DEFAULT 0,
  aligned_students INT NOT NULL DEFAULT 0,
  avg_match_score DECIMAL(6,3) NOT NULL DEFAULT 0.000,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uk_major_snapshot (major_id, snapshot_date),
  INDEX idx_snapshot_date (snapshot_date),
  CONSTRAINT fk_snapshot_major
    FOREIGN KEY (major_id) REFERENCES majors(id)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB;
