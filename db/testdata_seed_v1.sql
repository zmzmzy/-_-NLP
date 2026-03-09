USE graduate_match;
SET NAMES utf8mb4;

-- =========================================================
-- 备用测试数据包 v1（可重复导入）
-- 覆盖范围：学院/专业/专业画像/学生/企业/岗位/就业记录/账号绑定
-- 设计原则：
-- 1) 使用独立编码前缀（TU_/TC_/TM_/2026T*）避免污染主数据
-- 2) 尽量使用 UPSERT，重复执行不报错
-- 3) 不 TRUNCATE，不清空现有业务数据
-- =========================================================

-- -------------------------
-- 0) 测试账号（密码统一：Demo@123）
-- sha256(Demo@123) = ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73
-- -------------------------
INSERT INTO users(username, email, password_hash, role, is_active)
VALUES
  ('demo_admin', 'demo_admin@test.local', 'sha256:ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73', 'admin', 1),
  ('demo_stu01', 'demo_stu01@test.local', 'sha256:ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73', 'viewer', 1),
  ('demo_stu02', 'demo_stu02@test.local', 'sha256:ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73', 'viewer', 1),
  ('demo_stu03', 'demo_stu03@test.local', 'sha256:ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73', 'viewer', 1),
  ('demo_stu04', 'demo_stu04@test.local', 'sha256:ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73', 'viewer', 1)
ON DUPLICATE KEY UPDATE
  email = VALUES(email),
  password_hash = VALUES(password_hash),
  role = VALUES(role),
  is_active = VALUES(is_active);

-- -------------------------
-- 1) 学校 / 学院 / 专业 / 专业画像
-- -------------------------
INSERT INTO universities(code, name, province, city, description)
VALUES ('TU_DEMO', '毕业设计测试大学', '北京市', '北京', '用于毕业设计系统联调与答辩演示的测试学校')
ON DUPLICATE KEY UPDATE
  name = VALUES(name),
  province = VALUES(province),
  city = VALUES(city),
  description = VALUES(description);

INSERT INTO colleges(university_id, code, name, description)
SELECT u.id, 'TC01', '智能制造与计算学院（测试）', '覆盖软件工程与机器人工程方向'
FROM universities u
WHERE u.code = 'TU_DEMO'
ON DUPLICATE KEY UPDATE
  university_id = VALUES(university_id),
  description = VALUES(description);

INSERT INTO colleges(university_id, code, name, description)
SELECT u.id, 'TC02', '交通与车辆学院（测试）', '覆盖车辆工程与新能源车辆方向'
FROM universities u
WHERE u.code = 'TU_DEMO'
ON DUPLICATE KEY UPDATE
  university_id = VALUES(university_id),
  description = VALUES(description);

INSERT INTO colleges(university_id, code, name, description)
SELECT u.id, 'TC03', '商业与数据学院（测试）', '覆盖数据科学与市场营销方向'
FROM universities u
WHERE u.code = 'TU_DEMO'
ON DUPLICATE KEY UPDATE
  university_id = VALUES(university_id),
  description = VALUES(description);

INSERT INTO colleges(university_id, code, name, description)
SELECT u.id, 'TC04', '电子信息学院（测试）', '覆盖通信工程与物联网工程方向'
FROM universities u
WHERE u.code = 'TU_DEMO'
ON DUPLICATE KEY UPDATE
  university_id = VALUES(university_id),
  description = VALUES(description);

INSERT INTO majors(college_id, code, name, description)
SELECT c.id, 'TM01', '软件工程（测试）', '培养软件系统分析设计、开发测试与工程协同能力'
FROM colleges c WHERE c.code = 'TC01'
ON DUPLICATE KEY UPDATE
  code = VALUES(code),
  description = VALUES(description);

INSERT INTO majors(college_id, code, name, description)
SELECT c.id, 'TM02', '机器人工程（测试）', '培养机器人算法、控制系统与工程部署能力'
FROM colleges c WHERE c.code = 'TC01'
ON DUPLICATE KEY UPDATE
  code = VALUES(code),
  description = VALUES(description);

INSERT INTO majors(college_id, code, name, description)
SELECT c.id, 'TM03', '车辆工程（测试）', '培养车载系统、整车电子与工程验证能力'
FROM colleges c WHERE c.code = 'TC02'
ON DUPLICATE KEY UPDATE
  code = VALUES(code),
  description = VALUES(description);

INSERT INTO majors(college_id, code, name, description)
SELECT c.id, 'TM04', '新能源汽车工程（测试）', '培养电驱动系统、BMS与整车标定能力'
FROM colleges c WHERE c.code = 'TC02'
ON DUPLICATE KEY UPDATE
  code = VALUES(code),
  description = VALUES(description);

INSERT INTO majors(college_id, code, name, description)
SELECT c.id, 'TM05', '数据科学与大数据技术（测试）', '培养数据治理、建模分析与数据产品化能力'
FROM colleges c WHERE c.code = 'TC03'
ON DUPLICATE KEY UPDATE
  code = VALUES(code),
  description = VALUES(description);

INSERT INTO majors(college_id, code, name, description)
SELECT c.id, 'TM06', '市场营销（测试）', '培养市场洞察、活动运营与业务增长能力'
FROM colleges c WHERE c.code = 'TC03'
ON DUPLICATE KEY UPDATE
  code = VALUES(code),
  description = VALUES(description);

INSERT INTO majors(college_id, code, name, description)
SELECT c.id, 'TM07', '通信工程（测试）', '培养移动通信、网络优化与协议分析能力'
FROM colleges c WHERE c.code = 'TC04'
ON DUPLICATE KEY UPDATE
  code = VALUES(code),
  description = VALUES(description);

INSERT INTO majors(college_id, code, name, description)
SELECT c.id, 'TM08', '物联网工程（测试）', '培养感知层设备、平台开发与行业落地能力'
FROM colleges c WHERE c.code = 'TC04'
ON DUPLICATE KEY UPDATE
  code = VALUES(code),
  description = VALUES(description);

INSERT INTO major_profiles(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
  '数据结构、操作系统、数据库系统、软件工程、计算机网络',
  '后端开发、接口设计、数据库建模、代码测试与重构能力',
  'Linux、Git、MySQL、C++/Java、Docker',
  '工程协作、问题分析、需求拆解、版本管理能力'
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code = 'TC01' AND m.name = '软件工程（测试）'
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

INSERT INTO major_profiles(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
  '机器人学、自动控制原理、机器视觉、传感器技术、嵌入式系统',
  '机器人路径规划、控制调参、视觉识别、系统集成与调试',
  'ROS、C++、Python、Linux、OpenCV',
  '系统联调、故障定位、现场部署、跨团队沟通能力'
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code = 'TC01' AND m.name = '机器人工程（测试）'
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

INSERT INTO major_profiles(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
  '汽车构造、车载电子、电控系统、CAN 总线、整车测试',
  '车载系统分析、控制策略实现、接口联调与故障诊断',
  'C、CANoe、MATLAB、嵌入式开发板',
  '工程验证、问题闭环、标准化测试能力'
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code = 'TC02' AND m.name = '车辆工程（测试）'
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

INSERT INTO major_profiles(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
  '动力电池原理、电机驱动、BMS、新能源整车控制',
  '电池状态估计、BMS 需求分析、控制策略验证',
  'Simulink、C、Python、HIL 平台',
  '数据分析、安全意识、工程落地能力'
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code = 'TC02' AND m.name = '新能源汽车工程（测试）'
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

INSERT INTO major_profiles(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
  '概率统计、机器学习、数据仓库、数据可视化、SQL',
  '数据清洗、特征构建、指标分析、模型评估与解释',
  'Python、pandas、scikit-learn、MySQL、BI',
  '分析思维、业务理解、结论表达能力'
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code = 'TC03' AND m.name = '数据科学与大数据技术（测试）'
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

INSERT INTO major_profiles(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
  '市场调研、消费者行为、数字营销、品牌管理、运营分析',
  '活动策划、投放分析、用户增长、跨部门协同',
  'Excel、BI、内容平台、CRM',
  '沟通表达、组织执行、数据驱动运营能力'
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code = 'TC03' AND m.name = '市场营销（测试）'
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

INSERT INTO major_profiles(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
  '通信原理、移动通信、网络规划、无线优化、协议分析',
  '网络问题定位、指标监控、站点优化与测试分析',
  'Wireshark、网络优化工具、Linux',
  '现场排障、性能优化、质量保障能力'
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code = 'TC04' AND m.name = '通信工程（测试）'
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

INSERT INTO major_profiles(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
  '传感器技术、嵌入式开发、物联网平台、云边协同、网络安全基础',
  '设备接入、数据采集、平台开发、行业场景对接',
  'C/C++、MQTT、Linux、MySQL、Docker',
  '系统集成、场景抽象、工程交付能力'
FROM majors m
JOIN colleges c ON c.id = m.college_id
WHERE c.code = 'TC04' AND m.name = '物联网工程（测试）'
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

-- -------------------------
-- 2) 企业与岗位
-- -------------------------
INSERT INTO companies(name, main_business, flagship_products, domain_area, application_industries, province, city, website, description)
VALUES
  ('星河智驾科技（测试）', '智能驾驶与车载计算平台研发', '自动驾驶域控平台', '智能汽车', '乘用车,物流车', '北京市', '北京', 'https://example.com/xinghe', '测试企业数据'),
  ('云岳工业软件（测试）', '工业数字化与制造执行系统研发', 'MES 执行平台', '工业软件', '离散制造,流程工业', '上海市', '上海', 'https://example.com/yunyue', '测试企业数据'),
  ('蓝桥新能源（测试）', '新能源动力系统研发与集成', 'BMS 控制器', '新能源车', '乘用车,储能', '安徽省', '合肥', 'https://example.com/lanqiao', '测试企业数据'),
  ('数栈洞察科技（测试）', '数据分析与商业洞察服务', '实时分析平台', '数据智能', '零售,互联网', '浙江省', '杭州', 'https://example.com/shuzhan', '测试企业数据'),
  ('华联零售集团（测试）', '连锁零售运营与供应链管理', '会员运营系统', '零售运营', '商超,电商', '广东省', '广州', 'https://example.com/hualian', '测试企业数据'),
  ('北斗物联系统（测试）', '物联网连接与通信网络优化服务', 'IoT 云管平台', '通信与物联', '智慧园区,工业互联网', '江苏省', '南京', 'https://example.com/beidou', '测试企业数据')
ON DUPLICATE KEY UPDATE
  main_business = VALUES(main_business),
  flagship_products = VALUES(flagship_products),
  domain_area = VALUES(domain_area),
  application_industries = VALUES(application_industries),
  province = VALUES(province),
  city = VALUES(city),
  website = VALUES(website),
  description = VALUES(description);

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, required_tools_platform_text, published_at)
SELECT c.id, 'C++后端工程师（测试）', 'full_time', '北京市', '北京', 'bachelor', '18k-28k',
  '负责服务端核心模块开发与性能优化',
  '参与车载数据平台和网关系统研发',
  '熟悉 C++/Linux/网络编程/数据库',
  'GCC,CMake,Git,MySQL',
  NOW()
FROM companies c
WHERE c.name = '星河智驾科技（测试）'
  AND NOT EXISTS (SELECT 1 FROM jobs j WHERE j.company_id = c.id AND j.title = 'C++后端工程师（测试）');

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, required_tools_platform_text, published_at)
SELECT c.id, '机器人算法工程师（测试）', 'full_time', '北京市', '北京', 'master', '22k-35k',
  '负责机器人感知与控制算法优化',
  '参与移动机器人路径规划项目',
  '熟悉 ROS、Python、C++、控制理论',
  'ROS,Python,C++,Linux',
  NOW()
FROM companies c
WHERE c.name = '星河智驾科技（测试）'
  AND NOT EXISTS (SELECT 1 FROM jobs j WHERE j.company_id = c.id AND j.title = '机器人算法工程师（测试）');

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, required_tools_platform_text, published_at)
SELECT c.id, 'AI视觉算法实习生（测试）', 'internship', '北京市', '北京', 'bachelor', '4k-6k',
  '参与视觉模型训练与数据标注质量优化',
  '参与仓储机器人目标检测实习项目',
  '了解深度学习基础与 Python 编程',
  'Python,PyTorch,Linux',
  NOW()
FROM companies c
WHERE c.name = '星河智驾科技（测试）'
  AND NOT EXISTS (SELECT 1 FROM jobs j WHERE j.company_id = c.id AND j.title = 'AI视觉算法实习生（测试）');

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, required_tools_platform_text, published_at)
SELECT c.id, '工业软件实施工程师（测试）', 'full_time', '上海市', '上海', 'bachelor', '12k-20k',
  '负责制造企业 MES 项目实施与交付',
  '参与设备联网与生产数据集成项目',
  '了解 SQL、业务流程建模、现场沟通能力',
  'SQL,Excel,实施工具包',
  NOW()
FROM companies c
WHERE c.name = '云岳工业软件（测试）'
  AND NOT EXISTS (SELECT 1 FROM jobs j WHERE j.company_id = c.id AND j.title = '工业软件实施工程师（测试）');

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, required_tools_platform_text, published_at)
SELECT c.id, '智能座舱嵌入式工程师（测试）', 'full_time', '安徽省', '合肥', 'bachelor', '16k-25k',
  '负责车载嵌入式软件模块开发与联调',
  '参与车机控制器量产项目',
  '熟悉 C、嵌入式系统、CAN 总线',
  'C,RTOS,CANoe',
  NOW()
FROM companies c
WHERE c.name = '蓝桥新能源（测试）'
  AND NOT EXISTS (SELECT 1 FROM jobs j WHERE j.company_id = c.id AND j.title = '智能座舱嵌入式工程师（测试）');

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, required_tools_platform_text, published_at)
SELECT c.id, '新能源BMS工程师（测试）', 'full_time', '安徽省', '合肥', 'bachelor', '18k-30k',
  '负责电池管理系统策略开发与验证',
  '参与电芯状态估计与安全策略项目',
  '熟悉 BMS 原理、控制算法、标定流程',
  'Matlab,Simulink,C',
  NOW()
FROM companies c
WHERE c.name = '蓝桥新能源（测试）'
  AND NOT EXISTS (SELECT 1 FROM jobs j WHERE j.company_id = c.id AND j.title = '新能源BMS工程师（测试）');

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, required_tools_platform_text, published_at)
SELECT c.id, '数据分析工程师（测试）', 'full_time', '浙江省', '杭州', 'bachelor', '14k-22k',
  '负责经营数据分析与可视化指标体系建设',
  '参与用户增长归因分析项目',
  '熟悉 SQL、Python、统计分析',
  'Python,SQL,BI',
  NOW()
FROM companies c
WHERE c.name = '数栈洞察科技（测试）'
  AND NOT EXISTS (SELECT 1 FROM jobs j WHERE j.company_id = c.id AND j.title = '数据分析工程师（测试）');

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, required_tools_platform_text, published_at)
SELECT c.id, '商业运营专员（测试）', 'full_time', '广东省', '广州', 'bachelor', '10k-16k',
  '负责活动运营与会员增长策略执行',
  '参与门店活动策划与复盘项目',
  '熟悉营销活动执行、数据报表分析',
  'Excel,BI,CRM',
  NOW()
FROM companies c
WHERE c.name = '华联零售集团（测试）'
  AND NOT EXISTS (SELECT 1 FROM jobs j WHERE j.company_id = c.id AND j.title = '商业运营专员（测试）');

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, required_tools_platform_text, published_at)
SELECT c.id, '5G网络优化工程师（测试）', 'full_time', '江苏省', '南京', 'bachelor', '15k-24k',
  '负责无线网络参数优化与故障定位',
  '参与城区站点优化与专项保障',
  '熟悉通信原理、网络优化流程与测试分析',
  '网络优化工具,Wireshark,Linux',
  NOW()
FROM companies c
WHERE c.name = '北斗物联系统（测试）'
  AND NOT EXISTS (SELECT 1 FROM jobs j WHERE j.company_id = c.id AND j.title = '5G网络优化工程师（测试）');

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, required_tools_platform_text, published_at)
SELECT c.id, '物联网平台开发工程师（测试）', 'full_time', '江苏省', '南京', 'bachelor', '16k-26k',
  '负责设备接入与平台服务模块开发',
  '参与园区物联平台建设项目',
  '熟悉 C++/Java、MQTT、数据库与微服务基础',
  'C++,Java,MQTT,MySQL,Docker',
  NOW()
FROM companies c
WHERE c.name = '北斗物联系统（测试）'
  AND NOT EXISTS (SELECT 1 FROM jobs j WHERE j.company_id = c.id AND j.title = '物联网平台开发工程师（测试）');

-- -------------------------
-- 3) 学生信息（16 人）
-- -------------------------
INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T001', '李晨阳', 'male', c.id, m.id, 2026, '2026t001@test.local', '13900010001',
  '主修软件工程，熟悉 C++、数据库、Linux，参与课程项目后端开发。',
  '校园综合服务平台后端开发与性能调优。',
  '编码规范意识强，具备团队协作与问题定位能力。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC01' AND m.name = '软件工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T002', '王诗涵', 'female', c.id, m.id, 2026, '2026t002@test.local', '13900010002',
  '掌握软件测试、接口设计与数据库建模，参与多个课程项目。',
  '制造执行系统数据采集模块开发。',
  '学习能力强，沟通表达清晰。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC01' AND m.name = '软件工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T003', '赵宇航', 'male', c.id, m.id, 2026, '2026t003@test.local', '13900010003',
  '主修机器人算法与控制，掌握 ROS、Python、C++。',
  '移动机器人路径规划与仿真。',
  '具备系统联调和算法优化能力。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC01' AND m.name = '机器人工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T004', '孙可欣', 'female', c.id, m.id, 2026, '2026t004@test.local', '13900010004',
  '熟悉机器人视觉感知与数据处理，具备算法工程实践基础。',
  '仓储机器人视觉识别实训。',
  '责任心强，执行力稳定。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC01' AND m.name = '机器人工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T005', '周嘉豪', 'male', c.id, m.id, 2026, '2026t005@test.local', '13900010005',
  '熟悉车载电子系统与嵌入式开发流程。',
  '车载控制器 CAN 通信联调项目。',
  '工程实践能力较好。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC02' AND m.name = '车辆工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T006', '郑雨桐', 'female', c.id, m.id, 2026, '2026t006@test.local', '13900010006',
  '掌握整车电子测试与故障定位。',
  '车机系统验证与问题闭环。',
  '沟通协作能力良好。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC02' AND m.name = '车辆工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T007', '何俊凯', 'male', c.id, m.id, 2026, '2026t007@test.local', '13900010007',
  '熟悉动力电池管理系统与状态估计算法。',
  'BMS 采样与故障策略验证。',
  '具备较强的问题分析能力。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC02' AND m.name = '新能源汽车工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T008', '陈若曦', 'female', c.id, m.id, 2026, '2026t008@test.local', '13900010008',
  '熟悉电驱与能量管理基础，掌握 Simulink 建模。',
  '动力系统参数标定课程项目。',
  '学习主动性强。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC02' AND m.name = '新能源汽车工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T009', '杨一鸣', 'male', c.id, m.id, 2026, '2026t009@test.local', '13900010009',
  '熟悉数据清洗、统计分析与建模评估流程。',
  '零售销量预测与可视化分析项目。',
  '数据敏感度高。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC03' AND m.name = '数据科学与大数据技术（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T010', '刘子妍', 'female', c.id, m.id, 2026, '2026t010@test.local', '13900010010',
  '熟悉 Python 与 SQL，具备指标体系搭建能力。',
  '用户分层分析与策略试验项目。',
  '逻辑清晰，表达能力好。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC03' AND m.name = '数据科学与大数据技术（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T011', '彭浩然', 'male', c.id, m.id, 2026, '2026t011@test.local', '13900010011',
  '掌握市场调研、活动运营与业务分析基础。',
  '校园品牌活动策划与执行项目。',
  '执行力强，组织协调能力较好。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC03' AND m.name = '市场营销（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T012', '许嘉宁', 'female', c.id, m.id, 2026, '2026t012@test.local', '13900010012',
  '具备活动策划与数据复盘能力。',
  '线上促销活动执行与转化分析。',
  '沟通和服务意识强。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC03' AND m.name = '市场营销（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T013', '马瑞泽', 'male', c.id, m.id, 2026, '2026t013@test.local', '13900010013',
  '掌握移动通信基础、网络优化流程与问题定位能力。',
  '通信网络 KPI 分析与优化实践。',
  '现场支持与抗压能力较好。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC04' AND m.name = '通信工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T014', '田婧怡', 'female', c.id, m.id, 2026, '2026t014@test.local', '13900010014',
  '熟悉无线网络测试与日志分析。',
  '站点优化与性能监控课程项目。',
  '细致认真，结果导向。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC04' AND m.name = '通信工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T015', '林博文', 'male', c.id, m.id, 2026, '2026t015@test.local', '13900010015',
  '掌握物联网设备接入、消息协议与平台开发基础。',
  '智慧教室设备接入与告警系统项目。',
  '系统集成思维较强。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC04' AND m.name = '物联网工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone, resume_text, technical_projects_text, capability_summary_text)
SELECT '2026T016', '方语晴', 'female', c.id, m.id, 2026, '2026t016@test.local', '13900010016',
  '熟悉设备管理平台与数据看板配置。',
  '园区环境监测设备数据接入实训。',
  '执行细致，文档规范意识好。'
FROM colleges c JOIN majors m ON m.college_id = c.id
WHERE c.code = 'TC04' AND m.name = '物联网工程（测试）'
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name), gender = VALUES(gender), college_id = VALUES(college_id), major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year), email = VALUES(email), phone = VALUES(phone),
  resume_text = VALUES(resume_text), technical_projects_text = VALUES(technical_projects_text), capability_summary_text = VALUES(capability_summary_text);

-- 账号绑定学生（只绑定 4 个账号用于学生端登录测试）
UPDATE users u JOIN students s ON s.student_no = '2026T001'
SET u.linked_student_id = s.id
WHERE u.username = 'demo_stu01';

UPDATE users u JOIN students s ON s.student_no = '2026T003'
SET u.linked_student_id = s.id
WHERE u.username = 'demo_stu02';

UPDATE users u JOIN students s ON s.student_no = '2026T009'
SET u.linked_student_id = s.id
WHERE u.username = 'demo_stu03';

UPDATE users u JOIN students s ON s.student_no = '2026T013'
SET u.linked_student_id = s.id
WHERE u.username = 'demo_stu04';

-- -------------------------
-- 4) 就业记录（每位学生 1 条，用于匹配与看板）
-- -------------------------
INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'signed', 'active', '2026-07-10', NULL, '三方协议', '对口样本'
FROM students s
JOIN companies c ON c.name = '星河智驾科技（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = 'C++后端工程师（测试）'
WHERE s.student_no = '2026T001'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'offer', 'active', '2026-07-15', NULL, 'offer邮件', '对口样本'
FROM students s
JOIN companies c ON c.name = '云岳工业软件（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '工业软件实施工程师（测试）'
WHERE s.student_no = '2026T002'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'signed', 'active', '2026-07-08', NULL, '三方协议', '对口样本'
FROM students s
JOIN companies c ON c.name = '星河智驾科技（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '机器人算法工程师（测试）'
WHERE s.student_no = '2026T003'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'internship', 'active', '2026-06-20', NULL, '实习协议', '对口样本'
FROM students s
JOIN companies c ON c.name = '星河智驾科技（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = 'AI视觉算法实习生（测试）'
WHERE s.student_no = '2026T004'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'offer', 'active', '2026-07-11', NULL, 'offer邮件', '对口样本'
FROM students s
JOIN companies c ON c.name = '蓝桥新能源（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '智能座舱嵌入式工程师（测试）'
WHERE s.student_no = '2026T005'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'signed', 'active', '2026-07-18', NULL, '三方协议', '对口样本'
FROM students s
JOIN companies c ON c.name = '蓝桥新能源（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '智能座舱嵌入式工程师（测试）'
WHERE s.student_no = '2026T006'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'signed', 'active', '2026-07-07', NULL, '三方协议', '对口样本'
FROM students s
JOIN companies c ON c.name = '蓝桥新能源（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '新能源BMS工程师（测试）'
WHERE s.student_no = '2026T007'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'offer', 'active', '2026-07-20', NULL, 'offer邮件', '对口样本'
FROM students s
JOIN companies c ON c.name = '蓝桥新能源（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '新能源BMS工程师（测试）'
WHERE s.student_no = '2026T008'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'signed', 'active', '2026-07-05', NULL, '三方协议', '对口样本'
FROM students s
JOIN companies c ON c.name = '数栈洞察科技（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '数据分析工程师（测试）'
WHERE s.student_no = '2026T009'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'offer', 'active', '2026-07-09', NULL, 'offer邮件', '对口样本'
FROM students s
JOIN companies c ON c.name = '数栈洞察科技（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '数据分析工程师（测试）'
WHERE s.student_no = '2026T010'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'signed', 'active', '2026-07-13', NULL, '三方协议', '对口样本'
FROM students s
JOIN companies c ON c.name = '华联零售集团（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '商业运营专员（测试）'
WHERE s.student_no = '2026T011'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

-- 故意放入 1 条“偏不对口”样本（市场营销 -> 数据分析岗位）
INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'signed', 'active', '2026-07-16', NULL, '三方协议', '偏不对口样本'
FROM students s
JOIN companies c ON c.name = '数栈洞察科技（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '数据分析工程师（测试）'
WHERE s.student_no = '2026T012'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'signed', 'active', '2026-07-03', NULL, '三方协议', '对口样本'
FROM students s
JOIN companies c ON c.name = '北斗物联系统（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '5G网络优化工程师（测试）'
WHERE s.student_no = '2026T013'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'offer', 'active', '2026-07-12', NULL, 'offer邮件', '对口样本'
FROM students s
JOIN companies c ON c.name = '北斗物联系统（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '5G网络优化工程师（测试）'
WHERE s.student_no = '2026T014'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'signed', 'active', '2026-07-04', NULL, '三方协议', '对口样本'
FROM students s
JOIN companies c ON c.name = '北斗物联系统（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '物联网平台开发工程师（测试）'
WHERE s.student_no = '2026T015'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

-- 故意放入 1 条“偏不对口”样本（物联网 -> 商业运营）
INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, start_date, end_date, evidence_source, notes)
SELECT s.id, c.id, j.id, 'offer', 'active', '2026-07-19', NULL, 'offer邮件', '偏不对口样本'
FROM students s
JOIN companies c ON c.name = '华联零售集团（测试）'
JOIN jobs j ON j.company_id = c.id AND j.title = '商业运营专员（测试）'
WHERE s.student_no = '2026T016'
  AND NOT EXISTS (SELECT 1 FROM employment_records er WHERE er.student_id = s.id);

-- -------------------------
-- 5) 导入结果摘要
-- -------------------------
SELECT 'test_data_pack_v1_loaded' AS message;
SELECT COUNT(*) AS demo_colleges FROM colleges WHERE code LIKE 'TC%';
SELECT COUNT(*) AS demo_majors FROM majors m JOIN colleges c ON c.id = m.college_id WHERE c.code LIKE 'TC%';
SELECT COUNT(*) AS demo_students FROM students WHERE student_no LIKE '2026T%';
SELECT COUNT(*) AS demo_companies FROM companies WHERE name LIKE '%（测试）';
SELECT COUNT(*) AS demo_jobs FROM jobs WHERE title LIKE '%（测试）';
SELECT COUNT(*) AS demo_employments FROM employment_records er JOIN students s ON s.id = er.student_id WHERE s.student_no LIKE '2026T%';
