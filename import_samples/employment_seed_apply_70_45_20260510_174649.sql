USE graduate_match;
SET NAMES utf8mb4;
START TRANSACTION;

INSERT INTO users(username,email,password_hash,role,is_active)
VALUES('seed_submitter','seed_submitter@local','sha256:ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73','viewer',1)
ON DUPLICATE KEY UPDATE email=VALUES(email), password_hash=VALUES(password_hash), role=VALUES(role), is_active=VALUES(is_active);

SET @seed_submitter_id := (SELECT id FROM users WHERE username='seed_submitter' LIMIT 1);
SET @admin_id := (SELECT id FROM users WHERE role='admin' ORDER BY id ASC LIMIT 1);

DELETE FROM employment_submissions;
DELETE FROM employment_records;
DELETE FROM match_results;

DROP TEMPORARY TABLE IF EXISTS tmp_seed_plan;
CREATE TEMPORARY TABLE tmp_seed_plan (
  student_id BIGINT NOT NULL,
  student_no VARCHAR(64) NOT NULL,
  major_id BIGINT NOT NULL,
  job_id BIGINT NOT NULL,
  company_id BIGINT NOT NULL,
  scenario ENUM('approved','pending','rejected') NOT NULL,
  aligned TINYINT(1) NOT NULL DEFAULT 0,
  employment_type ENUM('signed','offer','internship','pending') NOT NULL DEFAULT 'signed',
  start_date DATE NOT NULL,
  semantic_score DECIMAL(6,3) NOT NULL DEFAULT 0.000,
  skill_score DECIMAL(6,3) NOT NULL DEFAULT 0.000,
  constraint_score DECIMAL(6,3) NOT NULL DEFAULT 1.000,
  final_score DECIMAL(6,3) NOT NULL DEFAULT 0.000,
  PRIMARY KEY(student_id)
) ENGINE=InnoDB;

INSERT INTO tmp_seed_plan(student_id,student_no,major_id,job_id,company_id,scenario,aligned,employment_type,start_date,semantic_score,skill_score,constraint_score,final_score)
VALUES
  (105,'222022321401001',1,1,1,'approved',1,'signed','2026-02-21',83.000,81.500,1.000,79.000),
  (106,'222022321401002',1,1,1,'approved',1,'signed','2026-02-22',91.000,89.500,1.000,86.000),
  (107,'222022321401003',1,4,4,'approved',0,'signed','2026-02-23',61.000,54.000,0.900,57.000),
  (108,'222022321401004',1,6,6,'approved',0,'signed','2026-02-24',67.000,56.000,1.000,62.000),
  (1,'222022321072037',2,3,3,'approved',1,'signed','2026-02-25',90.000,89.500,1.000,86.000),
  (111,'222022321402001',2,3,3,'approved',1,'offer','2026-02-26',77.000,73.500,1.000,72.000),
  (112,'222022321402002',2,3,3,'approved',1,'offer','2026-02-27',85.000,81.500,1.000,79.000),
  (113,'222022321402003',2,4,4,'approved',0,'offer','2026-02-28',61.000,49.000,1.000,55.000),
  (114,'222022321402004',2,6,6,'approved',0,'internship','2026-03-01',64.000,55.000,0.900,60.000),
  (115,'222022321402005',2,8,8,'approved',0,'signed','2026-03-02',70.000,61.000,1.000,65.000),
  (118,'222022321403001',3,4,4,'approved',1,'signed','2026-03-03',92.000,89.500,1.000,86.000),
  (119,'222022321403002',3,4,4,'approved',1,'signed','2026-03-04',75.000,73.500,1.000,72.000),
  (120,'222022321403003',3,4,4,'approved',1,'signed','2026-03-05',83.000,81.500,1.000,79.000),
  (121,'222022321403004',3,7,7,'approved',0,'signed','2026-03-06',64.000,54.000,1.000,58.000),
  (122,'222022321403005',3,9,9,'approved',0,'signed','2026-03-07',67.000,60.000,0.900,63.000),
  (123,'222022321403006',3,11,11,'approved',0,'offer','2026-03-08',73.000,62.000,1.000,68.000),
  (126,'222022321404001',4,12,12,'approved',1,'offer','2026-03-09',90.000,89.500,1.000,86.000),
  (127,'222022321404002',4,12,12,'approved',1,'offer','2026-03-10',77.000,73.500,1.000,72.000),
  (128,'222022321404003',4,12,12,'approved',1,'internship','2026-03-11',85.000,81.500,1.000,79.000),
  (129,'222022321404004',4,10,10,'approved',0,'signed','2026-03-12',67.000,55.000,1.000,61.000),
  (130,'222022321404005',4,1,1,'approved',0,'signed','2026-03-13',70.000,61.000,0.900,66.000),
  (131,'222022321404006',4,2,2,'approved',0,'signed','2026-03-14',49.000,40.000,1.000,44.000),
  (135,'222022322901001',5,6,6,'approved',1,'signed','2026-03-15',92.000,89.500,1.000,86.000),
  (136,'222022322901002',5,6,6,'approved',1,'signed','2026-03-16',75.000,73.500,1.000,72.000),
  (137,'222022322901003',5,6,6,'approved',1,'signed','2026-03-17',83.000,81.500,1.000,79.000),
  (138,'222022322901004',5,2,2,'approved',0,'offer','2026-03-18',70.000,60.000,1.000,64.000),
  (139,'222022322901005',5,4,4,'approved',0,'offer','2026-03-19',46.000,39.000,0.900,42.000),
  (140,'222022322901006',5,7,7,'approved',0,'offer','2026-03-20',52.000,41.000,1.000,47.000),
  (141,'222022322901007',5,8,8,'approved',0,'internship','2026-03-21',58.000,47.000,1.000,52.000),
  (145,'222022322902001',6,5,5,'approved',1,'signed','2026-03-22',77.000,73.500,1.000,72.000),
  (146,'222022322902002',6,5,5,'approved',1,'signed','2026-03-23',85.000,81.500,1.000,79.000),
  (147,'222022322902003',6,5,5,'approved',1,'signed','2026-03-24',89.000,89.500,1.000,86.000),
  (148,'222022322902004',6,8,8,'approved',0,'signed','2026-03-25',49.000,40.000,0.900,45.000),
  (149,'222022322902005',6,10,10,'approved',0,'signed','2026-03-26',55.000,46.000,1.000,50.000),
  (150,'222022322902006',6,12,12,'approved',0,'signed','2026-03-27',61.000,52.000,1.000,55.000),
  (151,'222022322902007',6,2,2,'approved',0,'offer','2026-03-28',64.000,54.000,0.900,60.000),
  (152,'222022322902008',6,4,4,'approved',0,'offer','2026-03-29',70.000,60.000,1.000,65.000),
  (156,'222022323301001',7,7,7,'approved',1,'offer','2026-03-30',91.000,89.500,1.000,86.000),
  (157,'222022323301002',7,7,7,'approved',1,'internship','2026-03-31',78.000,73.500,1.000,72.000),
  (158,'222022323301003',7,7,7,'approved',1,'signed','2026-04-01',82.000,81.500,1.000,79.000),
  (159,'222022323301004',7,4,4,'approved',0,'signed','2026-04-02',64.000,53.000,1.000,58.000),
  (160,'222022323301005',7,6,6,'approved',0,'signed','2026-04-03',67.000,59.000,0.900,63.000),
  (161,'222022323301006',7,8,8,'approved',0,'signed','2026-04-04',73.000,65.000,1.000,68.000),
  (162,'222022323301007',7,10,10,'approved',0,'signed','2026-04-05',52.000,40.000,1.000,46.000),
  (163,'222022323301008',7,12,12,'approved',0,'signed','2026-04-06',55.000,46.000,0.900,51.000),
  (168,'222022323302001',8,6,6,'approved',1,'offer','2026-04-07',84.000,81.500,1.000,79.000),
  (169,'222022323302002',8,6,6,'approved',1,'offer','2026-04-08',92.000,89.500,1.000,86.000),
  (170,'222022323302003',8,11,11,'approved',0,'offer','2026-04-09',70.000,60.000,0.900,66.000),
  (171,'222022323302004',8,1,1,'approved',0,'internship','2026-04-10',49.000,39.000,1.000,44.000),
  (174,'222022324001001',9,8,8,'approved',1,'signed','2026-04-11',91.000,89.500,1.000,86.000),
  (175,'222022324001002',9,8,8,'approved',1,'signed','2026-04-12',78.000,73.500,1.000,72.000),
  (176,'222022324001003',9,10,10,'approved',0,'signed','2026-04-13',64.000,53.000,1.000,59.000),
  (177,'222022324001004',9,12,12,'approved',0,'signed','2026-04-14',70.000,59.000,1.000,64.000),
  (178,'222022324001005',9,2,2,'approved',0,'signed','2026-04-15',46.000,38.000,0.900,42.000),
  (181,'222022324002001',10,9,9,'approved',1,'signed','2026-04-16',85.000,81.500,1.000,79.000),
  (182,'222022324002002',10,9,9,'approved',1,'offer','2026-04-17',89.000,89.500,1.000,86.000),
  (183,'222022324002003',10,9,9,'approved',1,'offer','2026-04-18',76.000,73.500,1.000,72.000),
  (184,'222022324002004',10,1,1,'approved',0,'offer','2026-04-19',67.000,58.000,1.000,62.000),
  (185,'222022324002005',10,3,3,'approved',0,'internship','2026-04-20',73.000,64.000,1.000,67.000),
  (186,'222022324002006',10,5,5,'approved',0,'signed','2026-04-21',49.000,39.000,0.900,45.000),
  (189,'222022325201001',11,10,10,'approved',1,'signed','2026-04-22',83.000,81.500,1.000,79.000),
  (190,'222022325201002',11,10,10,'approved',1,'signed','2026-04-23',91.000,89.500,1.000,86.000),
  (191,'222022325201003',11,10,10,'approved',1,'signed','2026-04-24',78.000,73.500,1.000,72.000),
  (192,'222022325201004',11,4,4,'approved',0,'signed','2026-04-25',70.000,59.000,1.000,65.000),
  (193,'222022325201005',11,6,6,'approved',0,'signed','2026-04-26',49.000,38.000,1.000,43.000),
  (194,'222022325201006',11,8,8,'approved',0,'offer','2026-04-27',52.000,44.000,0.900,48.000),
  (198,'222022325202001',12,11,11,'approved',1,'offer','2026-04-28',85.000,81.500,1.000,79.000),
  (199,'222022325202002',12,11,11,'approved',1,'offer','2026-04-29',89.000,89.500,1.000,86.000),
  (200,'222022325202003',12,11,11,'approved',1,'internship','2026-04-30',76.000,73.500,1.000,72.000),
  (201,'222022325202004',12,8,8,'approved',0,'signed','2026-05-01',73.000,64.000,1.000,68.000),
  (202,'222022325202005',12,10,10,'approved',0,'signed','2026-05-02',52.000,43.000,1.000,46.000),
  (203,'222022325202006',12,12,12,'approved',0,'signed','2026-05-03',55.000,45.000,0.900,51.000),
  (204,'222022325202007',12,2,2,'approved',0,'signed','2026-05-04',61.000,51.000,1.000,56.000),
  (109,'222022321401005',1,1,1,'pending',0,'signed','2026-05-12',0.000,0.000,1.000,0.000),
  (110,'222022321401006',1,1,1,'pending',0,'signed','2026-05-13',0.000,0.000,1.000,0.000),
  (116,'222022321402006',2,3,3,'pending',0,'signed','2026-05-14',0.000,0.000,1.000,0.000),
  (117,'222022321402007',2,3,3,'pending',0,'signed','2026-05-15',0.000,0.000,1.000,0.000),
  (124,'222022321403007',3,4,4,'pending',0,'signed','2026-05-16',0.000,0.000,1.000,0.000),
  (125,'222022321403008',3,4,4,'pending',0,'signed','2026-05-17',0.000,0.000,1.000,0.000),
  (132,'222022321404007',4,12,12,'pending',0,'signed','2026-05-18',0.000,0.000,1.000,0.000),
  (133,'222022321404008',4,12,12,'pending',0,'signed','2026-05-19',0.000,0.000,1.000,0.000),
  (134,'222022321404009',4,12,12,'pending',0,'signed','2026-05-20',0.000,0.000,1.000,0.000),
  (142,'222022322901008',5,7,7,'rejected',0,'offer','2026-06-01',0.000,0.000,1.000,0.000),
  (143,'222022322901009',5,2,2,'rejected',0,'offer','2026-06-02',0.000,0.000,1.000,0.000),
  (144,'222022322901010',5,10,10,'rejected',0,'offer','2026-06-03',0.000,0.000,1.000,0.000),
  (153,'222022322902009',6,2,2,'rejected',0,'offer','2026-06-04',0.000,0.000,1.000,0.000),
  (154,'222022322902010',6,10,10,'rejected',0,'offer','2026-06-05',0.000,0.000,1.000,0.000),
  (155,'222022322902011',6,6,6,'rejected',0,'offer','2026-06-06',0.000,0.000,1.000,0.000),
  (164,'222022323301009',7,10,10,'rejected',0,'offer','2026-06-07',0.000,0.000,1.000,0.000),
  (165,'222022323301010',7,6,6,'rejected',0,'offer','2026-06-08',0.000,0.000,1.000,0.000);

INSERT INTO employment_records(student_id,company_id,job_id,employment_type,employment_status,start_date,end_date,evidence_source,notes,created_at,updated_at)
SELECT p.student_id,p.company_id,p.job_id,p.employment_type,'active',p.start_date,NULL,'seed_70_45_v1',
       CONCAT('seed_approved_',p.student_no,': defense showcase baseline for 70/45 targets'), NOW(), NOW()
FROM tmp_seed_plan p
WHERE p.scenario='approved';

INSERT INTO employment_submissions(
  submitter_user_id,student_id,company_id,job_id,
  proposed_company_name,proposed_job_title,
  proposed_job_responsibilities_text,proposed_job_required_knowledge_skills_text,proposed_job_required_tools_platform_text,
  employment_type,employment_status,start_date,end_date,evidence_source,notes,status,
  review_comment,reviewed_by_user_id,reviewed_at,approved_employment_record_id,created_at,updated_at
)
SELECT @seed_submitter_id,p.student_id,p.company_id,p.job_id,c.name,j.title,
       'Seeded approved submission for defense showcase process demo.',
       'Skill profile validated in seeded scenario.',
       'Platform profile validated in seeded scenario.',
       p.employment_type,'active',p.start_date,NULL,'seed_70_45_v1',
       CONCAT('submission_approved_',p.student_no),
       'approved','approved by seeded admin flow',@admin_id,NOW(),er.id,NOW(),NOW()
FROM tmp_seed_plan p
JOIN employment_records er ON er.student_id=p.student_id
JOIN companies c ON c.id=p.company_id
JOIN jobs j ON j.id=p.job_id
WHERE p.scenario='approved';

INSERT INTO employment_submissions(
  submitter_user_id,student_id,company_id,job_id,
  proposed_company_name,proposed_job_title,
  proposed_job_responsibilities_text,proposed_job_required_knowledge_skills_text,proposed_job_required_tools_platform_text,
  employment_type,employment_status,start_date,end_date,evidence_source,notes,status,created_at,updated_at
)
SELECT @seed_submitter_id,p.student_id,p.company_id,p.job_id,c.name,j.title,
       'Seeded pending submission for live admin-review demo.',
       'Pending skill evidence check.',
       'Pending platform evidence check.',
       p.employment_type,'active',p.start_date,NULL,'seed_70_45_v1',
       CONCAT('submission_pending_',p.student_no),'pending',NOW(),NOW()
FROM tmp_seed_plan p
JOIN companies c ON c.id=p.company_id
JOIN jobs j ON j.id=p.job_id
WHERE p.scenario='pending';

INSERT INTO employment_submissions(
  submitter_user_id,student_id,company_id,job_id,
  proposed_company_name,proposed_job_title,
  proposed_job_responsibilities_text,proposed_job_required_knowledge_skills_text,proposed_job_required_tools_platform_text,
  employment_type,employment_status,start_date,end_date,evidence_source,notes,status,
  review_comment,reviewed_by_user_id,reviewed_at,created_at,updated_at
)
SELECT @seed_submitter_id,p.student_id,p.company_id,p.job_id,c.name,j.title,
       'Seeded rejected submission to demonstrate governance quality checks.',
       'Rejected due to missing required materials.',
       'Rejected due to evidence inconsistency.',
       p.employment_type,'active',p.start_date,NULL,'seed_70_45_v1',
       CONCAT('submission_rejected_',p.student_no),'rejected',
       'rejected in seeded admin review',@admin_id,NOW(),NOW(),NOW()
FROM tmp_seed_plan p
JOIN companies c ON c.id=p.company_id
JOIN jobs j ON j.id=p.job_id
WHERE p.scenario='rejected';

INSERT INTO match_results(student_id,major_id,job_id,algorithm_version,semantic_score,skill_score,constraint_score,final_score,matched_skills_json,missing_skills_json,constraint_failures_json,generated_at)
SELECT p.student_id,p.major_id,p.job_id,'v0.4-major-student',p.semantic_score,p.skill_score,p.constraint_score,p.final_score,
       JSON_ARRAY(),JSON_ARRAY(),JSON_ARRAY(),NOW()
FROM tmp_seed_plan p
WHERE p.scenario='approved';

COMMIT;
