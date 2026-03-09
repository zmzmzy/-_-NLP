-- v0.4 dashboard excellence score columns

USE graduate_match;
SET NAMES utf8mb4;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'major_alignment_snapshots'
    AND COLUMN_NAME = 'weighted_avg_match_score'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE major_alignment_snapshots ADD COLUMN weighted_avg_match_score DECIMAL(6,3) NOT NULL DEFAULT 0.000 AFTER avg_match_score',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'major_alignment_snapshots'
    AND COLUMN_NAME = 'excellent_graduate_score'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE major_alignment_snapshots ADD COLUMN excellent_graduate_score DECIMAL(6,3) NOT NULL DEFAULT 0.000 AFTER weighted_avg_match_score',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists := (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'major_alignment_snapshots'
    AND COLUMN_NAME = 'rank_position'
);
SET @sql := IF(
  @col_exists = 0,
  'ALTER TABLE major_alignment_snapshots ADD COLUMN rank_position INT NOT NULL DEFAULT 0 AFTER excellent_graduate_score',
  'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
