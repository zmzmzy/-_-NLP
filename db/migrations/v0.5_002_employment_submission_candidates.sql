-- v0.5 add candidate company/job fields for employment submissions
-- and allow company_id to be nullable until admin approval.

USE graduate_match;
SET NAMES utf8mb4;

SET @fk_exists := (
  SELECT COUNT(*)
  FROM information_schema.KEY_COLUMN_USAGE
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND CONSTRAINT_NAME = 'fk_emp_submission_company'
);
SET @sql := IF(
  @fk_exists > 0,
  'ALTER TABLE employment_submissions DROP FOREIGN KEY fk_emp_submission_company',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

ALTER TABLE employment_submissions
  MODIFY COLUMN company_id BIGINT NULL;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_company_name'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_company_name VARCHAR(255) NULL AFTER job_id',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_company_main_business'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_company_main_business TEXT NULL AFTER proposed_company_name',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_company_flagship_products'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_company_flagship_products TEXT NULL AFTER proposed_company_main_business',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_company_domain_area'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_company_domain_area TEXT NULL AFTER proposed_company_flagship_products',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_company_application_industries'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_company_application_industries TEXT NULL AFTER proposed_company_domain_area',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_company_province'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_company_province VARCHAR(64) NULL AFTER proposed_company_application_industries',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_company_city'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_company_city VARCHAR(64) NULL AFTER proposed_company_province',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_job_title'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_job_title VARCHAR(255) NULL AFTER proposed_company_city',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_job_type'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_job_type ENUM(''full_time'', ''internship'', ''contract'') NULL AFTER proposed_job_title',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_job_min_degree'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_job_min_degree ENUM(''none'', ''diploma'', ''bachelor'', ''master'', ''phd'') NULL AFTER proposed_job_type',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_job_salary_range'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_job_salary_range VARCHAR(64) NULL AFTER proposed_job_min_degree',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_job_location_province'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_job_location_province VARCHAR(64) NULL AFTER proposed_job_salary_range',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_job_location_city'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_job_location_city VARCHAR(64) NULL AFTER proposed_job_location_province',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_job_responsibilities_text'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_job_responsibilities_text TEXT NULL AFTER proposed_job_location_city',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_job_technical_projects_text'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_job_technical_projects_text TEXT NULL AFTER proposed_job_responsibilities_text',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_job_required_knowledge_skills_text'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_job_required_knowledge_skills_text TEXT NULL AFTER proposed_job_technical_projects_text',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND COLUMN_NAME = 'proposed_job_required_tools_platform_text'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE employment_submissions ADD COLUMN proposed_job_required_tools_platform_text TEXT NULL AFTER proposed_job_required_knowledge_skills_text',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @fk_exists := (
  SELECT COUNT(*)
  FROM information_schema.KEY_COLUMN_USAGE
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'employment_submissions'
    AND CONSTRAINT_NAME = 'fk_emp_submission_company'
);
SET @sql := IF(
  @fk_exists = 0,
  'ALTER TABLE employment_submissions ADD CONSTRAINT fk_emp_submission_company FOREIGN KEY (company_id) REFERENCES companies(id) ON DELETE SET NULL ON UPDATE CASCADE',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
