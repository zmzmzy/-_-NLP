USE graduate_match;

-- No default users are created in seed_minimal.sql.
-- Please register an account through /api/auth/register,
-- then promote one account to admin manually if needed.

-- Colleges (12 samples)
INSERT INTO colleges (code, name, description) VALUES
('C01', 'College of Computer Science', 'Computing and software related majors'),
('C02', 'College of Electronic Engineering', 'Electronics and communication majors'),
('C03', 'College of Mechanical Engineering', 'Mechanical design and manufacturing'),
('C04', 'College of Civil Engineering', 'Civil engineering and construction'),
('C05', 'College of Business Administration', 'Management and business majors'),
('C06', 'College of Economics', 'Economics and finance majors'),
('C07', 'College of Foreign Languages', 'Language and translation majors'),
('C08', 'College of Chemistry and Materials', 'Chemistry and materials science'),
('C09', 'College of Life Sciences', 'Biology and life sciences'),
('C10', 'College of Environmental Science', 'Environment and sustainability majors'),
('C11', 'College of Mathematics and Statistics', 'Math and statistics majors'),
('C12', 'College of Education', 'Education and pedagogy majors')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- Majors (sample starter set, extend to your final dataset)
INSERT INTO majors (college_id, code, name, description)
SELECT c.id, 'M001', 'Software Engineering', 'Software lifecycle, architecture, and engineering practice'
FROM colleges c WHERE c.code = 'C01'
ON DUPLICATE KEY UPDATE description = VALUES(description);

INSERT INTO majors (college_id, code, name, description)
SELECT c.id, 'M002', 'Data Science', 'Data analysis, machine learning, and data systems'
FROM colleges c WHERE c.code = 'C11'
ON DUPLICATE KEY UPDATE description = VALUES(description);

INSERT INTO majors (college_id, code, name, description)
SELECT c.id, 'M003', 'Electronic Information Engineering', 'Signals, circuits, embedded systems, communication'
FROM colleges c WHERE c.code = 'C02'
ON DUPLICATE KEY UPDATE description = VALUES(description);

INSERT INTO majors (college_id, code, name, description)
SELECT c.id, 'M004', 'Mechanical Engineering', 'Mechanical design, manufacturing, automation basics'
FROM colleges c WHERE c.code = 'C03'
ON DUPLICATE KEY UPDATE description = VALUES(description);

INSERT INTO majors (college_id, code, name, description)
SELECT c.id, 'M005', 'Marketing', 'Marketing strategy, consumer behavior, digital marketing'
FROM colleges c WHERE c.code = 'C05'
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- Major profiles for starter majors
INSERT INTO major_profiles (major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
  'Data structure, operating systems, database systems, software engineering, computer networks',
  'Software development, system design, database modeling, API integration, software testing',
  'Linux, Git, MySQL, Docker, Java/Spring or C++ frameworks',
  'Engineering practice, teamwork, problem solving, requirement analysis'
FROM majors m WHERE m.code = 'M001'
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

INSERT INTO major_profiles (major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
  'Probability statistics, machine learning, data mining, database, big data processing',
  'Data preprocessing, feature engineering, model training, visualization, SQL analysis',
  'Python, pandas, scikit-learn, Jupyter, MySQL, BI tools',
  'Data reasoning, analytical thinking, experiment design, report writing'
FROM majors m WHERE m.code = 'M002'
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

-- Skills dictionary starter
INSERT INTO skills (name, category) VALUES
('Python', 'programming'),
('C++', 'programming'),
('Java', 'programming'),
('SQL', 'database'),
('Machine Learning', 'ai'),
('Spring Boot', 'framework'),
('Docker', 'devops'),
('Data Analysis', 'analytics')
ON DUPLICATE KEY UPDATE category = VALUES(category);
