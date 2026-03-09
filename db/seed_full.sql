USE graduate_match;
SET NAMES utf8mb4;

-- =========================================================
-- 完整样本种子数据（毕业设计 v0.1）
-- 目标：覆盖 10+ 学院、20+ 专业画像、每专业至少 1 名学生与就业样本
-- 注意：本脚本会清空业务数据表后重建样本
-- =========================================================

SET FOREIGN_KEY_CHECKS = 0;
TRUNCATE TABLE major_alignment_snapshots;
TRUNCATE TABLE match_results;
TRUNCATE TABLE employment_records;
TRUNCATE TABLE job_skills;
TRUNCATE TABLE major_skills;
TRUNCATE TABLE skills;
TRUNCATE TABLE students;
TRUNCATE TABLE jobs;
TRUNCATE TABLE companies;
TRUNCATE TABLE major_profiles;
TRUNCATE TABLE majors;
TRUNCATE TABLE colleges;
TRUNCATE TABLE system_logs;
SET FOREIGN_KEY_CHECKS = 1;

-- 不再写入默认账号。
-- 请先通过 /api/auth/register 注册账号，再手动将一个账号提升为 admin。

-- 12 个学院
INSERT INTO colleges (code, name, description) VALUES
('C01', '计算机学院', '面向软件工程、计算机科学、网络工程等方向培养工程人才'),
('C02', '电子信息学院', '覆盖电子、通信、信号处理与嵌入式系统方向'),
('C03', '机械工程学院', '聚焦机械设计、制造与自动化装备方向'),
('C04', '土木与交通学院', '覆盖土木工程、工程管理与建设数字化方向'),
('C05', '管理学院', '聚焦市场、组织管理与企业管理实践'),
('C06', '经济金融学院', '覆盖金融、贸易与产业经济分析方向'),
('C07', '外国语学院', '面向英语、翻译与跨文化沟通方向'),
('C08', '化学与材料学院', '覆盖化学分析、材料研发与工艺优化方向'),
('C09', '生命科学学院', '聚焦生物技术、实验技能与生物数据分析'),
('C10', '环境与安全学院', '覆盖环境监测、污染治理与绿色工程方向'),
('C11', '数学与统计学院', '聚焦统计建模、数据科学与量化分析'),
('C12', '教育学院', '覆盖教育技术、课程设计与儿童发展方向');

-- 24 个专业（20+）
INSERT INTO majors (college_id, code, name, description)
SELECT c.id, x.code, x.name, x.description
FROM colleges c
JOIN (
  SELECT 'C01' AS college_code, 'M001' AS code, '软件工程' AS name, 'bachelor' AS legacy_tag, '工学' AS discipline_category,
         '面向复杂软件系统开发、测试、运维与项目协同能力培养' AS description
  UNION ALL SELECT 'C01', 'M002', '计算机科学与技术', 'bachelor', '工学', '面向计算机系统、算法设计与工程实现能力培养'
  UNION ALL SELECT 'C01', 'M003', '网络工程', 'bachelor', '工学', '面向网络架构、安全防护与运维优化能力培养'
  UNION ALL SELECT 'C11', 'M004', '数据科学与大数据技术', 'bachelor', '工学', '面向数据治理、机器学习与数据产品化能力培养'
  UNION ALL SELECT 'C11', 'M005', '应用统计学', 'bachelor', '理学', '面向统计建模、实验设计与行业数据决策能力培养'
  UNION ALL SELECT 'C02', 'M006', '电子信息工程', 'bachelor', '工学', '面向电子系统设计、信号处理与嵌入式应用能力培养'
  UNION ALL SELECT 'C02', 'M007', '通信工程', 'bachelor', '工学', '面向移动通信、协议实现与网络优化能力培养'
  UNION ALL SELECT 'C03', 'M008', '机械工程', 'bachelor', '工学', '面向机械结构设计、仿真分析与制造实践能力培养'
  UNION ALL SELECT 'C03', 'M009', '智能制造工程', 'bachelor', '工学', '面向产线自动化、工业机器人与数字化制造能力培养'
  UNION ALL SELECT 'C03', 'M010', '自动化', 'bachelor', '工学', '面向控制系统、PLC与工业控制工程能力培养'
  UNION ALL SELECT 'C04', 'M011', '土木工程', 'bachelor', '工学', '面向土木结构设计、施工组织与BIM协同能力培养'
  UNION ALL SELECT 'C04', 'M012', '工程管理', 'bachelor', '管理学', '面向工程计划、成本与进度协同管理能力培养'
  UNION ALL SELECT 'C05', 'M013', '市场营销', 'bachelor', '管理学', '面向市场洞察、品牌运营与增长策略能力培养'
  UNION ALL SELECT 'C05', 'M014', '人力资源管理', 'bachelor', '管理学', '面向组织发展、人才评价与绩效管理能力培养'
  UNION ALL SELECT 'C06', 'M015', '金融学', 'bachelor', '经济学', '面向金融产品分析、风险控制与量化决策能力培养'
  UNION ALL SELECT 'C06', 'M016', '国际经济与贸易', 'bachelor', '经济学', '面向国际贸易运营、跨境商务与市场拓展能力培养'
  UNION ALL SELECT 'C07', 'M017', '英语', 'bachelor', '文学', '面向商务沟通、跨文化协作与语言服务能力培养'
  UNION ALL SELECT 'C07', 'M018', '翻译', 'bachelor', '文学', '面向专业翻译、本地化与语言技术协同能力培养'
  UNION ALL SELECT 'C08', 'M019', '应用化学', 'bachelor', '理学', '面向化学检测、实验分析与工艺优化能力培养'
  UNION ALL SELECT 'C08', 'M020', '材料科学与工程', 'bachelor', '工学', '面向材料制备、性能评估与应用开发能力培养'
  UNION ALL SELECT 'C09', 'M021', '生物技术', 'bachelor', '理学', '面向生物实验、分子检测与生物数据处理能力培养'
  UNION ALL SELECT 'C10', 'M022', '环境工程', 'bachelor', '工学', '面向污染治理、环境监测与工程实施能力培养'
  UNION ALL SELECT 'C12', 'M023', '教育技术学', 'bachelor', '教育学', '面向数字课程设计、教学平台应用与学习分析能力培养'
  UNION ALL SELECT 'C12', 'M024', '学前教育', 'bachelor', '教育学', '面向儿童发展、课程实施与家园共育能力培养'
) x ON x.college_code = c.code;

-- 24 个专业画像文本（覆盖课程、知识技能、平台、能力）
INSERT INTO major_profiles (major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id, p.core_courses_text, p.knowledge_skills_text, p.technical_platform_text, p.capability_text
FROM majors m
JOIN (
  SELECT 'M001' AS major_code,
         '核心课程：程序设计基础、数据结构、操作系统、数据库系统、软件工程、软件测试。' AS core_courses_text,
         '知识技能：面向对象设计、后端开发、接口规范、数据库建模、持续集成。' AS knowledge_skills_text,
         '技术平台：Linux、Git、MySQL、Docker、Spring Boot、RESTful API。' AS technical_platform_text,
         '能力培养：需求分析、团队协作、版本管理、代码质量控制与工程交付。' AS capability_text
  UNION ALL SELECT 'M002', '核心课程：离散数学、计算机组成原理、编译原理、算法设计、计算机网络。',
         '知识技能：系统编程、算法优化、并发处理、网络协议分析、性能调优。',
         '技术平台：C++、Java、Linux、SQL、网络抓包工具。',
         '能力培养：抽象建模、复杂问题求解、工程实现与技术文档输出。'
  UNION ALL SELECT 'M003', '核心课程：计算机网络、网络安全、路由交换、网络管理、脚本自动化。',
         '知识技能：网络架构设计、安全加固、渗透测试基础、自动化运维。',
         '技术平台：Linux、Python、防火墙设备、监控平台、安全扫描工具。',
         '能力培养：故障定位、应急响应、规范化运维与风险评估。'
  UNION ALL SELECT 'M004', '核心课程：概率统计、数据挖掘、机器学习、分布式计算、数据仓库。',
         '知识技能：数据清洗、特征工程、模型训练、报表分析、数据产品思维。',
         '技术平台：Python、SQL、Docker、Notebook、可视化工具。',
         '能力培养：数据驱动决策、实验验证、跨团队沟通与结果复盘。'
  UNION ALL SELECT 'M005', '核心课程：数理统计、回归分析、时间序列、抽样调查、多元统计分析。',
         '知识技能：统计建模、假设检验、数据解释、风险分析与量化评估。',
         '技术平台：Python、R、SQL、统计分析软件。',
         '能力培养：严谨推断、指标设计、实验设计与结论表达。'
  UNION ALL SELECT 'M006', '核心课程：电路分析、模拟电子技术、数字信号处理、嵌入式系统。',
         '知识技能：信号处理、硬件调试、嵌入式开发、电子系统集成。',
         '技术平台：C++、嵌入式开发板、示波器、通信协议栈。',
         '能力培养：软硬件协同、实验验证、问题定位与工程实现。'
  UNION ALL SELECT 'M007', '核心课程：通信原理、移动通信、无线网络、数字通信、通信网络优化。',
         '知识技能：通信协议分析、网络优化、参数调优、通信系统测试。',
         '技术平台：Python、网络测试平台、频谱分析设备、Linux。',
         '能力培养：系统调测、指标分析、现场协作与方案迭代。'
  UNION ALL SELECT 'M008', '核心课程：机械制图、机械设计、材料力学、机械制造技术、有限元分析。',
         '知识技能：机械结构设计、强度校核、工艺评估、制造协同。',
         '技术平台：CAD、CAE、PDM、工程图纸规范。',
         '能力培养：结构创新、工程复核、标准化表达与项目协同。'
  UNION ALL SELECT 'M009', '核心课程：智能制造系统、工业机器人、PLC 控制、机电一体化。',
         '知识技能：产线自动化、机器人调试、控制策略配置、现场实施。',
         '技术平台：PLC、工业机器人平台、SCADA、MES。',
         '能力培养：系统集成、现场交付、跨专业协作与持续改进。'
  UNION ALL SELECT 'M010', '核心课程：自动控制原理、传感器技术、过程控制、电气控制系统。',
         '知识技能：控制建模、PLC 编程、自动化调试、设备联动。',
         '技术平台：PLC、组态软件、Python、嵌入式控制器。',
         '能力培养：控制方案设计、调试验证、故障分析与优化。'
  UNION ALL SELECT 'M011', '核心课程：结构力学、混凝土结构、工程测量、施工技术、BIM 基础。',
         '知识技能：土木设计、测量放样、工程图识读、施工组织。',
         '技术平台：BIM、CAD、测量设备、工程管理软件。',
         '能力培养：工程实施、质量安全意识、现场协同与进度管控。'
  UNION ALL SELECT 'M012', '核心课程：工程经济学、工程项目管理、合同管理、成本控制。',
         '知识技能：项目计划、资源协调、成本测算、风险管理。',
         '技术平台：BIM、项目管理软件、数据分析工具。',
         '能力培养：统筹组织、沟通协调、项目复盘与决策支持。'
  UNION ALL SELECT 'M013', '核心课程：市场营销学、消费者行为学、品牌管理、数字营销。',
         '知识技能：市场分析、品牌运营、活动策划、增长策略设计。',
         '技术平台：数据分析平台、内容运营工具、CRM。',
         '能力培养：洞察提炼、创意表达、跨部门协同与效果评估。'
  UNION ALL SELECT 'M014', '核心课程：人力资源管理、组织行为学、绩效管理、薪酬管理。',
         '知识技能：岗位分析、人才发展、绩效体系设计、员工沟通。',
         '技术平台：HR 信息系统、数据分析工具、流程管理平台。',
         '能力培养：组织协调、制度执行、数据决策与服务意识。'
  UNION ALL SELECT 'M015', '核心课程：金融学、投资学、公司金融、金融风险管理、计量经济学。',
         '知识技能：金融建模、风险评估、产品分析、量化思维。',
         '技术平台：Python、SQL、金融数据库、可视化工具。',
         '能力培养：风险意识、模型解释、策略评估与合规思维。'
  UNION ALL SELECT 'M016', '核心课程：国际贸易实务、国际结算、跨境电商、国际商务谈判。',
         '知识技能：贸易流程执行、商务沟通、市场拓展、单证处理。',
         '技术平台：跨境电商平台、ERP、数据分析工具。',
         '能力培养：跨文化沟通、执行落地、业务协同与问题处理。'
  UNION ALL SELECT 'M017', '核心课程：综合英语、商务英语、跨文化交际、英语写作。',
         '知识技能：商务沟通、英文写作、资料整理、外语服务。',
         '技术平台：办公协作平台、语言学习平台、文档协作工具。',
         '能力培养：语言表达、跨文化理解、沟通协调与学习迁移。'
  UNION ALL SELECT 'M018', '核心课程：翻译理论与实践、口译、笔译、CAT 工具应用。',
         '知识技能：术语管理、本地化翻译、语料处理、质量审校。',
         '技术平台：CAT 工具、术语库、语料库平台。',
         '能力培养：语言转换、规范意识、项目管理与交付质量控制。'
  UNION ALL SELECT 'M019', '核心课程：分析化学、有机化学、仪器分析、化工原理。',
         '知识技能：化学检测、实验设计、数据记录、安全规范执行。',
         '技术平台：实验室检测设备、LIMS、数据分析工具。',
         '能力培养：实验严谨性、规范操作、问题溯源与质量意识。'
  UNION ALL SELECT 'M020', '核心课程：材料科学基础、材料制备、材料表征、材料性能测试。',
         '知识技能：材料研发、工艺优化、性能分析、实验验证。',
         '技术平台：材料分析设备、仿真软件、实验平台。',
         '能力培养：研发思维、实验迭代、工程转化与协同沟通。'
  UNION ALL SELECT 'M021', '核心课程：生物化学、分子生物学、细胞工程、生物信息基础。',
         '知识技能：生物实验操作、分子检测、实验记录、数据分析。',
         '技术平台：PCR 平台、实验管理系统、统计分析工具。',
         '能力培养：实验复现、科学思维、规范化记录与团队协作。'
  UNION ALL SELECT 'M022', '核心课程：环境监测、环境工程学、污染控制工程、环境影响评价。',
         '知识技能：监测方案制定、数据分析、治理方案评估、工程实施。',
         '技术平台：环境监测设备、GIS、数据报送平台。',
         '能力培养：系统分析、现场执行、合规意识与沟通表达。'
  UNION ALL SELECT 'M023', '核心课程：教学系统设计、教育信息化、多媒体课件开发、学习科学。',
         '知识技能：教学设计、课程开发、学习数据分析、平台运营。',
         '技术平台：学习管理系统、前端开发框架、数据分析工具。',
         '能力培养：教学创新、产品化思维、用户沟通与迭代优化。'
  UNION ALL SELECT 'M024', '核心课程：学前儿童发展心理学、幼儿课程设计、游戏化教学、家园共育。',
         '知识技能：儿童观察评估、课程组织实施、沟通反馈、活动设计。',
         '技术平台：教育资源平台、家校沟通平台、课程管理工具。',
         '能力培养：儿童发展支持、同理沟通、组织协调与反思改进。'
) p ON p.major_code = m.code;

-- 企业样本（覆盖主业、拳头产品、领域与应用行业）
INSERT INTO companies (
  name, main_business, flagship_products, domain_area, application_industries,
  province, city, website, description
) VALUES
('华云科技股份有限公司', '企业级云原生平台研发与交付服务', '华云 DevCloud 平台、容器管理平台', '云计算与企业软件', '互联网、制造业、政务数字化', '北京', '北京', 'https://www.huayun.example.com', '为企业提供从应用开发到运维的一体化平台'),
('智源数据服务有限公司', '数据治理与智能分析解决方案', 'DataBrain 数据中台、智能分析引擎', '大数据与人工智能', '金融、零售、物流、教育', '上海', '上海', 'https://www.zhiyuan-data.example.com', '聚焦数据资产化与智能决策'),
('星链通信技术有限公司', '移动通信网络建设与优化', '5G 网优平台、协议分析系统', '通信与网络基础设施', '运营商、工业互联网、智慧园区', '广东', '深圳', 'https://www.xinglian-com.example.com', '服务于通信网络部署与优化运营'),
('远航智能装备有限公司', '工业装备设计与产线自动化', '智能装配单元、机器人工作站', '智能制造', '汽车、3C 电子、家电', '江苏', '苏州', 'https://www.yuanhang-equip.example.com', '提供自动化产线改造与实施服务'),
('城建数智工程集团', '工程设计与建设数字化管理', 'BIM 协同平台、智慧工地系统', '土木工程与工程数字化', '建筑、市政、交通基建', '湖北', '武汉', 'https://www.citybuild-digi.example.com', '提供从设计到施工管理的数字化方案'),
('博瑞金融科技有限公司', '金融数据产品与风险管理系统', '量化风控平台、智能投研系统', '金融科技', '银行、证券、保险', '北京', '北京', 'https://www.borui-fintech.example.com', '聚焦数据驱动的金融业务创新'),
('海纳市场咨询有限公司', '品牌与增长咨询服务', '品牌增长策略工具箱、用户洞察报告体系', '市场咨询', '消费品、互联网服务、教育', '浙江', '杭州', 'https://www.haina-marketing.example.com', '为企业提供品牌增长与组织能力建设服务'),
('新语教育科技有限公司', '教育平台与课程内容研发', '双语课程平台、学习资源管理系统', '教育科技', '基础教育、职业教育、企业培训', '四川', '成都', 'https://www.xinyu-edu.example.com', '以技术与内容融合提升教学效果'),
('清源环保科技有限公司', '环境监测与治理工程服务', '环境监测云平台、废水治理模块', '环保工程', '化工园区、市政环保、制造业', '江苏', '南京', 'https://www.qingyuan-env.example.com', '提供监测到治理的一体化环保服务'),
('绿洲生物医药有限公司', '生物实验与检测试剂研发', '细胞检测套件、PCR 试剂盒', '生物医药', '医疗检测、科研机构、药企', '上海', '上海', 'https://www.luzhou-bio.example.com', '面向生物实验与产业化应用'),
('晶材新材料有限公司', '高分子与复合材料研发制造', '轻量化复合材料板材、特种聚合物', '新材料', '汽车、电子封装、医疗器械', '安徽', '合肥', 'https://www.jingcai-material.example.com', '聚焦高性能材料研发与工程落地'),
('安澜网络安全有限公司', '网络攻防与安全运营服务', '态势感知平台、漏洞扫描系统', '网络安全', '政企信息化、互联网平台、金融机构', '广东', '广州', 'https://www.anlan-sec.example.com', '提供安全评估、监测与应急响应服务'),
('纵横物流科技有限公司', '跨境物流与供应链数字化', '跨境订单协同平台、仓配可视化系统', '供应链与物流科技', '跨境电商、制造业外贸', '浙江', '宁波', 'https://www.zongheng-logi.example.com', '通过数字化提升跨境履约效率'),
('智慧医疗信息有限公司', '医疗信息系统实施与运维', '临床数据中台、医院信息集成平台', '医疗信息化', '公立医院、连锁医疗机构', '湖南', '长沙', 'https://www.smart-med.example.com', '面向医疗机构的信息系统建设与优化'),
('天枢工业自动化有限公司', '工业控制系统集成与实施', '自动化控制平台、机器视觉质检系统', '工业自动化', '离散制造、流程工业、仓储物流', '重庆', '重庆', 'https://www.tianshu-auto.example.com', '提供工业控制与智能制造实施服务'),
('青禾幼教发展集团', '学前教育课程研发与师训服务', '幼儿发展课程包、家园共育平台', '学前教育', '幼儿园、托育机构、社区教育', '山东', '青岛', 'https://www.qinghe-kids.example.com', '聚焦学前课程与教师专业发展');

-- 岗位样本（包含职责、技术项目、知识技能、平台要求）
INSERT INTO jobs (
  company_id, title, job_type, location_province, location_city, min_degree,
  salary_range, responsibilities_text, technical_projects_text,
  required_knowledge_skills_text, required_tools_platform_text, published_at
)
SELECT c.id,
       j.title,
       j.job_type,
       j.location_province,
       j.location_city,
       j.min_degree,
       j.salary_range,
       j.responsibilities_text,
       j.technical_projects_text,
       j.required_knowledge_skills_text,
       j.required_tools_platform_text,
       NOW()
FROM companies c
JOIN (
  SELECT '华云科技股份有限公司' AS company_name, '后端开发工程师' AS title, 'full_time' AS job_type,
         '北京' AS location_province, '北京' AS location_city, 'bachelor' AS min_degree, '18k-28k' AS salary_range,
         '负责企业级业务系统后端模块设计、编码与接口联调。' AS responsibilities_text,
         '参与多租户平台微服务改造与高并发接口优化项目。' AS technical_projects_text,
         '掌握 Java、SQL、数据库建模、接口设计与基本系统设计能力。' AS required_knowledge_skills_text,
         '熟悉 Linux、Spring Boot、MySQL、Docker、Git。' AS required_tools_platform_text,
         'open' AS status
  UNION ALL SELECT '华云科技股份有限公司', '平台运维工程师', 'full_time', '北京', '北京', 'bachelor', '15k-24k',
         '负责容器平台日常运维、监控告警处理与发布支持。',
         '参与云原生平台稳定性治理与自动化运维脚本建设。',
         '掌握 Linux 运维、容器基础、故障排查与脚本开发能力。',
         '熟悉 Linux、Docker、Kubernetes、Python、监控系统。',
         'open'
  UNION ALL SELECT '智源数据服务有限公司', '数据分析工程师', 'full_time', '上海', '上海', 'bachelor', '16k-26k',
         '负责业务数据清洗、指标体系建设与经营分析报告输出。',
         '参与零售业务增长分析与用户分层画像项目。',
         '掌握 SQL、统计分析、数据建模与可视化表达能力。',
         '熟悉 Python、SQL、可视化工具、数据仓库。',
         'open'
  UNION ALL SELECT '智源数据服务有限公司', '机器学习工程师', 'full_time', '上海', '上海', 'bachelor', '20k-32k',
         '负责推荐与预测模型开发、上线评估与持续迭代。',
         '参与多场景模型训练平台与推理服务优化项目。',
         '掌握机器学习基础、特征工程、模型评估与工程部署能力。',
         '熟悉 Python、机器学习框架、SQL、Docker。',
         'open'
  UNION ALL SELECT '星链通信技术有限公司', '5G通信算法工程师', 'full_time', '广东', '深圳', 'bachelor', '18k-30k',
         '负责无线链路性能评估与通信参数优化。',
         '参与 5G 场景协议优化与链路质量提升项目。',
         '掌握通信原理、信号处理、算法实现与调测能力。',
         '熟悉 C++、Python、通信协议分析工具。',
         'open'
  UNION ALL SELECT '星链通信技术有限公司', '网络优化工程师', 'full_time', '广东', '深圳', 'bachelor', '14k-22k',
         '负责基站网络质量评估、故障分析与优化方案执行。',
         '参与城区网络覆盖与容量优化专项。',
         '掌握网络优化方法、指标分析与自动化脚本能力。',
         '熟悉通信测试平台、Python、Linux。',
         'open'
  UNION ALL SELECT '远航智能装备有限公司', '机械结构工程师', 'full_time', '江苏', '苏州', 'bachelor', '14k-23k',
         '负责设备结构设计、图纸输出与装配问题闭环。',
         '参与自动化装备结构轻量化改造项目。',
         '掌握机械设计原理、CAD 建模与力学分析能力。',
         '熟悉 CAD、有限元分析工具、PDM。',
         'open'
  UNION ALL SELECT '远航智能装备有限公司', '智能产线实施工程师', 'full_time', '江苏', '苏州', 'bachelor', '15k-24k',
         '负责产线现场实施、联调与交付验收支持。',
         '参与机器人产线升级与节拍优化项目。',
         '掌握 PLC、工业机器人、自动化调试与现场协同能力。',
         '熟悉 PLC 编程软件、机器人控制平台、MES。',
         'open'
  UNION ALL SELECT '城建数智工程集团', '土木设计工程师', 'full_time', '湖北', '武汉', 'bachelor', '13k-21k',
         '负责结构建模、施工图深化与设计变更处理。',
         '参与市政道路与配套建筑数字化设计项目。',
         '掌握土木结构设计、测量基础、工程图规范。',
         '熟悉 BIM、CAD、测量软件。',
         'open'
  UNION ALL SELECT '城建数智工程集团', '工程项目管理专员', 'full_time', '湖北', '武汉', 'bachelor', '12k-20k',
         '负责项目计划、进度跟踪、成本与风险台账管理。',
         '参与智慧工地项目多方协同管理实践。',
         '掌握项目管理、成本控制、数据汇总与沟通协调能力。',
         '熟悉项目管理软件、BIM 协同平台、办公套件。',
         'open'
  UNION ALL SELECT '博瑞金融科技有限公司', '金融数据产品经理', 'full_time', '北京', '北京', 'bachelor', '20k-35k',
         '负责金融数据产品需求分析、方案设计与迭代管理。',
         '参与投研与风控一体化数据产品建设项目。',
         '掌握金融业务知识、数据分析与产品设计能力。',
         '熟悉 SQL、数据分析平台、项目协同工具。',
         'open'
  UNION ALL SELECT '博瑞金融科技有限公司', '量化风控分析师', 'full_time', '北京', '北京', 'bachelor', '18k-30k',
         '负责风险指标建模、策略评估与异常监测。',
         '参与信贷风险预警与策略回测项目。',
         '掌握统计建模、风险控制、金融数据处理能力。',
         '熟悉 Python、SQL、量化分析工具。',
         'open'
  UNION ALL SELECT '海纳市场咨询有限公司', '市场策划专员', 'full_time', '浙江', '杭州', 'bachelor', '11k-18k',
         '负责品牌活动策划、用户调研与传播方案落地。',
         '参与新消费品牌增长策略项目。',
         '掌握市场分析、品牌运营、数据复盘与沟通表达能力。',
         '熟悉内容运营工具、数据分析平台。',
         'open'
  UNION ALL SELECT '海纳市场咨询有限公司', '人才发展专员', 'full_time', '浙江', '杭州', 'bachelor', '11k-17k',
         '负责人才盘点、培训实施与绩效数据整理。',
         '参与组织发展与人才梯队建设项目。',
         '掌握人力资源管理、绩效分析、沟通协作能力。',
         '熟悉 HR 系统、数据分析工具。',
         'open'
  UNION ALL SELECT '纵横物流科技有限公司', '跨境贸易运营专员', 'full_time', '浙江', '宁波', 'bachelor', '12k-20k',
         '负责跨境订单执行、供应链协同与客户需求响应。',
         '参与跨境履约效率提升与流程优化项目。',
         '掌握国际贸易流程、数据分析与业务协同能力。',
         '熟悉 ERP、跨境电商平台、报表工具。',
         'open'
  UNION ALL SELECT '纵横物流科技有限公司', '商务英语客户经理', 'full_time', '浙江', '宁波', 'bachelor', '12k-19k',
         '负责海外客户沟通、方案讲解与服务交付跟进。',
         '参与重点客户全流程商务支持项目。',
         '掌握商务英语表达、跨文化沟通与客户管理能力。',
         '熟悉 CRM、协同办公平台。',
         'open'
  UNION ALL SELECT '新语教育科技有限公司', '本地化翻译工程师', 'full_time', '四川', '成都', 'bachelor', '12k-20k',
         '负责教育产品多语种本地化翻译与术语维护。',
         '参与课程平台国际化版本上线项目。',
         '掌握翻译技术、语料处理与质量审校能力。',
         '熟悉 CAT 工具、术语管理平台。',
         'open'
  UNION ALL SELECT '新语教育科技有限公司', '双语课程教研专员', 'full_time', '四川', '成都', 'bachelor', '11k-18k',
         '负责双语课程内容开发、教学反馈分析与迭代。',
         '参与双语课程体系建设与教师培训项目。',
         '掌握教学设计、课程开发、双语沟通与评估能力。',
         '熟悉教学平台、课程管理系统。',
         'open'
  UNION ALL SELECT '晶材新材料有限公司', '化学检测工程师', 'full_time', '安徽', '合肥', 'bachelor', '12k-19k',
         '负责样品检测、实验记录与质量问题排查。',
         '参与新材料样本检测标准化项目。',
         '掌握化学分析、实验设计、安全规范与数据记录能力。',
         '熟悉实验检测设备、LIMS。',
         'open'
  UNION ALL SELECT '晶材新材料有限公司', '材料研发工程师', 'full_time', '安徽', '合肥', 'bachelor', '15k-25k',
         '负责新材料配方试验、性能评估与工艺优化。',
         '参与高性能复合材料开发项目。',
         '掌握材料科学基础、实验分析、研发迭代能力。',
         '熟悉材料实验平台、仿真分析工具。',
         'open'
  UNION ALL SELECT '绿洲生物医药有限公司', '生物实验技术员', 'full_time', '上海', '上海', 'bachelor', '11k-18k',
         '负责实验执行、样本处理与实验数据归档。',
         '参与分子检测流程标准化项目。',
         '掌握生物实验操作、分子检测与数据整理能力。',
         '熟悉 PCR 平台、实验管理系统。',
         'open'
  UNION ALL SELECT '清源环保科技有限公司', '环境监测工程师', 'full_time', '江苏', '南京', 'bachelor', '12k-20k',
         '负责监测点位布设、数据分析与监测报告输出。',
         '参与园区污染物在线监测项目。',
         '掌握环境监测、GIS 应用、数据分析与现场实施能力。',
         '熟悉监测设备、GIS、报表平台。',
         'open'
  UNION ALL SELECT '新语教育科技有限公司', '教育技术产品专员', 'full_time', '四川', '成都', 'bachelor', '12k-19k',
         '负责教学产品需求梳理、功能设计与上线协同。',
         '参与课堂互动平台优化与学习数据分析项目。',
         '掌握教学设计、产品协作、数据分析与用户沟通能力。',
         '熟悉课程平台、前端基础、分析工具。',
         'open'
  UNION ALL SELECT '青禾幼教发展集团', '学前教育课程专员', 'full_time', '山东', '青岛', 'bachelor', '10k-16k',
         '负责学前课程活动方案设计、实施支持与家校反馈。',
         '参与幼儿综合能力发展课程研发项目。',
         '掌握儿童心理、课程开发、教学组织与沟通能力。',
         '熟悉课程资源平台、家园共育平台。',
         'open'
  UNION ALL SELECT '安澜网络安全有限公司', '网络安全工程师', 'full_time', '广东', '广州', 'bachelor', '16k-27k',
         '负责安全评估、漏洞验证与安全加固方案落地。',
         '参与企业级安全运营与应急响应项目。',
         '掌握网络安全、渗透测试、脚本能力与风险分析能力。',
         '熟悉 Linux、安全扫描工具、日志分析平台。',
         'open'
  UNION ALL SELECT '智慧医疗信息有限公司', '医疗信息实施工程师', 'full_time', '湖南', '长沙', 'bachelor', '13k-21k',
         '负责医疗系统部署、流程配置与用户培训支持。',
         '参与医院临床数据平台实施项目。',
         '掌握项目实施、SQL、业务沟通与问题闭环能力。',
         '熟悉临床信息系统、数据库、工单平台。',
         'open'
  UNION ALL SELECT '天枢工业自动化有限公司', '自动化控制工程师', 'full_time', '重庆', '重庆', 'bachelor', '15k-24k',
         '负责自动化控制方案设计、PLC 调试与联动测试。',
         '参与仓储自动化改造与控制系统升级项目。',
         '掌握自动控制、PLC、现场调试与故障定位能力。',
         '熟悉 PLC、工业控制网络、调试工具。',
         'open'
  UNION ALL SELECT '天枢工业自动化有限公司', '工业机器视觉工程师', 'full_time', '重庆', '重庆', 'bachelor', '18k-28k',
         '负责机器视觉算法调试、缺陷检测模型优化与部署。',
         '参与产线视觉质检系统建设项目。',
         '掌握图像处理、机器学习、工业自动化协同能力。',
         '熟悉 Python、C++、视觉开发框架、工业相机平台。',
         'open'
) j ON j.company_name = c.name;

-- 技能词典
INSERT INTO skills (name, category, synonyms_text) VALUES
('Java', 'programming', 'java8,java11'),
('C++', 'programming', 'cpp'),
('Python', 'programming', 'py'),
('Go', 'programming', 'golang'),
('SQL', 'database', 'structured query language'),
('MySQL', 'database', 'mysql8'),
('PostgreSQL', 'database', 'postgres,pgsql'),
('Linux', 'platform', 'ubuntu,centos'),
('Docker', 'devops', 'container'),
('Kubernetes', 'devops', 'k8s'),
('Spring Boot', 'framework', 'springboot'),
('Vue.js', 'frontend', 'vue'),
('机器学习', 'ai', 'machine learning,ml'),
('深度学习', 'ai', 'deep learning,dl'),
('数据分析', 'analytics', 'data analysis'),
('统计建模', 'analytics', 'statistical modeling'),
('网络安全', 'security', 'cyber security'),
('渗透测试', 'security', 'pentest'),
('信号处理', 'communication', 'dsp'),
('嵌入式开发', 'embedded', 'embedded system'),
('通信协议', 'communication', 'tcp/ip,5g protocol'),
('网络优化', 'communication', 'network optimization'),
('机械设计', 'mechanical', 'mechanical design'),
('CAD', 'mechanical', 'computer aided design'),
('有限元分析', 'mechanical', 'fea'),
('PLC', 'automation', 'programmable logic controller'),
('工业机器人', 'automation', 'robotics'),
('自动控制', 'automation', 'control system'),
('BIM', 'civil', 'building information modeling'),
('工程测量', 'civil', 'surveying'),
('项目管理', 'management', 'project management'),
('成本控制', 'management', 'cost control'),
('市场分析', 'business', 'market analysis'),
('品牌运营', 'business', 'brand operation'),
('人力资源管理', 'management', 'hrm'),
('薪酬绩效', 'management', 'compensation and performance'),
('商务沟通', 'business', 'business communication'),
('金融建模', 'finance', 'financial modeling'),
('风险控制', 'finance', 'risk control'),
('国际贸易', 'business', 'international trade'),
('商务英语', 'language', 'business english'),
('翻译技术', 'language', 'translation technology'),
('CATTI规范', 'language', 'catti standards'),
('跨文化沟通', 'language', 'cross-cultural communication'),
('语料处理', 'language', 'corpus processing'),
('化学分析', 'chemistry', 'chemical analysis'),
('实验设计', 'science', 'experimental design'),
('安全规范', 'science', 'safety standard'),
('高分子材料', 'material', 'polymer'),
('生物实验', 'biology', 'bio experiment'),
('分子生物学', 'biology', 'molecular biology'),
('环境监测', 'environment', 'environmental monitoring'),
('GIS', 'environment', 'geographic information system'),
('教学设计', 'education', 'instructional design'),
('课程开发', 'education', 'curriculum development'),
('教育测评', 'education', 'educational assessment'),
('儿童心理', 'education', 'child psychology'),
('沟通协作', 'softskill', 'communication collaboration'),
('临床信息系统', 'healthcare', 'his,emr');

-- 专业技能映射
INSERT INTO major_skills (major_id, skill_id, weight, source_course)
SELECT m.id, s.id, x.weight, x.source_course
FROM majors m
JOIN skills s
JOIN (
  SELECT 'M001' AS major_code, 'Java' AS skill_name, 1.30 AS weight, '程序设计基础' AS source_course
  UNION ALL SELECT 'M001', 'SQL', 1.20, '数据库系统'
  UNION ALL SELECT 'M001', 'Spring Boot', 1.10, 'Web 开发实践'
  UNION ALL SELECT 'M001', 'Linux', 1.00, '操作系统'
  UNION ALL SELECT 'M001', 'Docker', 0.90, '软件部署实践'

  UNION ALL SELECT 'M002', 'C++', 1.20, '程序设计'
  UNION ALL SELECT 'M002', 'Java', 1.10, '面向对象程序设计'
  UNION ALL SELECT 'M002', 'SQL', 1.00, '数据库系统'
  UNION ALL SELECT 'M002', 'Linux', 0.90, '操作系统'
  UNION ALL SELECT 'M002', '网络安全', 0.90, '计算机网络安全'

  UNION ALL SELECT 'M003', '网络安全', 1.20, '网络安全'
  UNION ALL SELECT 'M003', '通信协议', 1.20, '网络协议分析'
  UNION ALL SELECT 'M003', 'Linux', 1.00, '网络操作系统'
  UNION ALL SELECT 'M003', 'Python', 0.90, '自动化运维'
  UNION ALL SELECT 'M003', '渗透测试', 0.80, '安全攻防实践'

  UNION ALL SELECT 'M004', 'Python', 1.30, '机器学习'
  UNION ALL SELECT 'M004', 'SQL', 1.20, '数据仓库'
  UNION ALL SELECT 'M004', '数据分析', 1.20, '数据挖掘'
  UNION ALL SELECT 'M004', '机器学习', 1.10, '机器学习'
  UNION ALL SELECT 'M004', 'Docker', 0.80, '模型部署'

  UNION ALL SELECT 'M005', '统计建模', 1.30, '数理统计'
  UNION ALL SELECT 'M005', 'Python', 1.10, '统计计算'
  UNION ALL SELECT 'M005', '数据分析', 1.10, '应用统计'
  UNION ALL SELECT 'M005', 'SQL', 0.90, '数据处理'
  UNION ALL SELECT 'M005', '风险控制', 0.80, '统计风险分析'

  UNION ALL SELECT 'M006', '信号处理', 1.20, '数字信号处理'
  UNION ALL SELECT 'M006', '嵌入式开发', 1.10, '嵌入式系统'
  UNION ALL SELECT 'M006', 'C++', 1.00, '程序设计'
  UNION ALL SELECT 'M006', '通信协议', 1.00, '通信原理'
  UNION ALL SELECT 'M006', '自动控制', 0.80, '控制基础'

  UNION ALL SELECT 'M007', '通信协议', 1.30, '通信原理'
  UNION ALL SELECT 'M007', '信号处理', 1.10, '数字通信'
  UNION ALL SELECT 'M007', '网络优化', 1.00, '网络优化'
  UNION ALL SELECT 'M007', 'Python', 0.90, '通信数据分析'
  UNION ALL SELECT 'M007', 'Linux', 0.80, '通信系统'

  UNION ALL SELECT 'M008', '机械设计', 1.20, '机械设计'
  UNION ALL SELECT 'M008', 'CAD', 1.20, '机械制图'
  UNION ALL SELECT 'M008', '有限元分析', 1.00, '结构分析'
  UNION ALL SELECT 'M008', '项目管理', 0.80, '工程实践'
  UNION ALL SELECT 'M008', '工业机器人', 0.80, '制造自动化'

  UNION ALL SELECT 'M009', 'PLC', 1.20, 'PLC 控制技术'
  UNION ALL SELECT 'M009', '工业机器人', 1.10, '机器人系统'
  UNION ALL SELECT 'M009', '自动控制', 1.00, '自动控制原理'
  UNION ALL SELECT 'M009', '机械设计', 0.90, '机电一体化'
  UNION ALL SELECT 'M009', 'Python', 0.80, '智能制造数据分析'

  UNION ALL SELECT 'M010', '自动控制', 1.20, '自动控制原理'
  UNION ALL SELECT 'M010', 'PLC', 1.10, '电气控制'
  UNION ALL SELECT 'M010', 'Python', 0.90, '控制算法'
  UNION ALL SELECT 'M010', '嵌入式开发', 0.90, '嵌入式控制'
  UNION ALL SELECT 'M010', '工业机器人', 0.80, '自动化产线'

  UNION ALL SELECT 'M011', 'BIM', 1.20, 'BIM 建模'
  UNION ALL SELECT 'M011', '工程测量', 1.20, '工程测量'
  UNION ALL SELECT 'M011', 'CAD', 1.00, '工程制图'
  UNION ALL SELECT 'M011', '项目管理', 0.90, '施工组织'
  UNION ALL SELECT 'M011', '安全规范', 0.80, '工程安全'

  UNION ALL SELECT 'M012', '项目管理', 1.20, '项目管理'
  UNION ALL SELECT 'M012', '成本控制', 1.10, '工程经济学'
  UNION ALL SELECT 'M012', 'BIM', 0.90, '工程信息化'
  UNION ALL SELECT 'M012', '数据分析', 0.90, '工程数据分析'
  UNION ALL SELECT 'M012', '沟通协作', 0.80, '项目沟通'

  UNION ALL SELECT 'M013', '市场分析', 1.20, '市场营销学'
  UNION ALL SELECT 'M013', '品牌运营', 1.10, '品牌管理'
  UNION ALL SELECT 'M013', '数据分析', 1.00, '营销数据分析'
  UNION ALL SELECT 'M013', '商务沟通', 0.90, '商务谈判'
  UNION ALL SELECT 'M013', '项目管理', 0.80, '营销项目实践'

  UNION ALL SELECT 'M014', '人力资源管理', 1.30, '人力资源管理'
  UNION ALL SELECT 'M014', '薪酬绩效', 1.10, '绩效管理'
  UNION ALL SELECT 'M014', '数据分析', 0.90, '人力数据分析'
  UNION ALL SELECT 'M014', '商务沟通', 0.90, '组织行为学'
  UNION ALL SELECT 'M014', '项目管理', 0.70, '管理实践'

  UNION ALL SELECT 'M015', '金融建模', 1.20, '金融工程'
  UNION ALL SELECT 'M015', '风险控制', 1.20, '金融风险管理'
  UNION ALL SELECT 'M015', '统计建模', 1.00, '计量经济学'
  UNION ALL SELECT 'M015', 'SQL', 0.90, '金融数据分析'
  UNION ALL SELECT 'M015', '数据分析', 0.90, '投资分析'

  UNION ALL SELECT 'M016', '国际贸易', 1.20, '国际贸易实务'
  UNION ALL SELECT 'M016', '商务英语', 1.00, '商务英语'
  UNION ALL SELECT 'M016', '市场分析', 1.00, '国际市场营销'
  UNION ALL SELECT 'M016', '数据分析', 0.80, '贸易数据分析'
  UNION ALL SELECT 'M016', '项目管理', 0.70, '贸易项目实践'

  UNION ALL SELECT 'M017', '商务英语', 1.20, '商务英语'
  UNION ALL SELECT 'M017', '跨文化沟通', 1.10, '跨文化交际'
  UNION ALL SELECT 'M017', '商务沟通', 0.90, '口语表达'
  UNION ALL SELECT 'M017', '课程开发', 0.70, '语言教学实践'
  UNION ALL SELECT 'M017', '项目管理', 0.60, '综合实践'

  UNION ALL SELECT 'M018', '翻译技术', 1.30, '翻译实践'
  UNION ALL SELECT 'M018', 'CATTI规范', 1.20, '翻译标准'
  UNION ALL SELECT 'M018', '商务英语', 0.90, '商务翻译'
  UNION ALL SELECT 'M018', '语料处理', 0.90, '语料库应用'
  UNION ALL SELECT 'M018', '项目管理', 0.70, '翻译项目管理'

  UNION ALL SELECT 'M019', '化学分析', 1.30, '分析化学'
  UNION ALL SELECT 'M019', '实验设计', 1.10, '实验方法学'
  UNION ALL SELECT 'M019', '安全规范', 1.00, '实验室安全'
  UNION ALL SELECT 'M019', '数据分析', 0.80, '实验数据处理'
  UNION ALL SELECT 'M019', 'Python', 0.60, '化学计算'

  UNION ALL SELECT 'M020', '高分子材料', 1.20, '高分子化学'
  UNION ALL SELECT 'M020', '化学分析', 1.00, '材料表征'
  UNION ALL SELECT 'M020', '有限元分析', 0.90, '材料力学'
  UNION ALL SELECT 'M020', '实验设计', 0.90, '材料实验'
  UNION ALL SELECT 'M020', '机械设计', 0.70, '结构设计'

  UNION ALL SELECT 'M021', '生物实验', 1.30, '生物实验技术'
  UNION ALL SELECT 'M021', '分子生物学', 1.20, '分子生物学'
  UNION ALL SELECT 'M021', '实验设计', 1.00, '实验设计'
  UNION ALL SELECT 'M021', '数据分析', 0.80, '生物统计'
  UNION ALL SELECT 'M021', 'Python', 0.60, '生物信息基础'

  UNION ALL SELECT 'M022', '环境监测', 1.20, '环境监测'
  UNION ALL SELECT 'M022', 'GIS', 1.00, '环境信息系统'
  UNION ALL SELECT 'M022', '化学分析', 1.00, '环境化学'
  UNION ALL SELECT 'M022', '数据分析', 0.90, '环境数据分析'
  UNION ALL SELECT 'M022', '项目管理', 0.80, '工程实施'

  UNION ALL SELECT 'M023', '教学设计', 1.20, '教学系统设计'
  UNION ALL SELECT 'M023', '课程开发', 1.10, '课程开发'
  UNION ALL SELECT 'M023', '教育测评', 1.00, '学习评价'
  UNION ALL SELECT 'M023', '数据分析', 0.90, '学习分析'
  UNION ALL SELECT 'M023', 'Vue.js', 0.70, '教育应用开发'

  UNION ALL SELECT 'M024', '儿童心理', 1.20, '儿童发展心理学'
  UNION ALL SELECT 'M024', '课程开发', 1.10, '幼儿课程设计'
  UNION ALL SELECT 'M024', '教学设计', 1.00, '教育活动设计'
  UNION ALL SELECT 'M024', '教育测评', 0.90, '发展评估'
  UNION ALL SELECT 'M024', '沟通协作', 0.80, '家园共育实践'
) x ON x.major_code = m.code AND x.skill_name = s.name;

-- 岗位技能映射
INSERT INTO job_skills (job_id, skill_id, weight, is_mandatory)
SELECT j.id, s.id, x.weight, x.is_mandatory
FROM jobs j
JOIN skills s
JOIN (
  SELECT '后端开发工程师' AS job_title, 'Java' AS skill_name, 1.30 AS weight, 1 AS is_mandatory
  UNION ALL SELECT '后端开发工程师', 'Spring Boot', 1.20, 1
  UNION ALL SELECT '后端开发工程师', 'SQL', 1.10, 1
  UNION ALL SELECT '后端开发工程师', 'Linux', 0.90, 0
  UNION ALL SELECT '后端开发工程师', 'Docker', 0.90, 0

  UNION ALL SELECT '平台运维工程师', 'Linux', 1.30, 1
  UNION ALL SELECT '平台运维工程师', 'Docker', 1.10, 1
  UNION ALL SELECT '平台运维工程师', 'Kubernetes', 1.20, 1
  UNION ALL SELECT '平台运维工程师', 'Python', 0.90, 0
  UNION ALL SELECT '平台运维工程师', '网络安全', 0.80, 0

  UNION ALL SELECT '数据分析工程师', 'Python', 1.20, 1
  UNION ALL SELECT '数据分析工程师', 'SQL', 1.20, 1
  UNION ALL SELECT '数据分析工程师', '数据分析', 1.20, 1
  UNION ALL SELECT '数据分析工程师', '统计建模', 1.00, 0
  UNION ALL SELECT '数据分析工程师', '项目管理', 0.70, 0

  UNION ALL SELECT '机器学习工程师', 'Python', 1.20, 1
  UNION ALL SELECT '机器学习工程师', '机器学习', 1.30, 1
  UNION ALL SELECT '机器学习工程师', '深度学习', 1.10, 1
  UNION ALL SELECT '机器学习工程师', 'SQL', 0.90, 0
  UNION ALL SELECT '机器学习工程师', 'Docker', 0.80, 0

  UNION ALL SELECT '5G通信算法工程师', '通信协议', 1.20, 1
  UNION ALL SELECT '5G通信算法工程师', '信号处理', 1.20, 1
  UNION ALL SELECT '5G通信算法工程师', 'C++', 1.10, 1
  UNION ALL SELECT '5G通信算法工程师', 'Python', 0.80, 0
  UNION ALL SELECT '5G通信算法工程师', '嵌入式开发', 0.80, 0

  UNION ALL SELECT '网络优化工程师', '通信协议', 1.20, 1
  UNION ALL SELECT '网络优化工程师', '网络优化', 1.20, 1
  UNION ALL SELECT '网络优化工程师', 'Python', 0.90, 0
  UNION ALL SELECT '网络优化工程师', '数据分析', 0.90, 0
  UNION ALL SELECT '网络优化工程师', 'Linux', 0.80, 0

  UNION ALL SELECT '机械结构工程师', '机械设计', 1.20, 1
  UNION ALL SELECT '机械结构工程师', 'CAD', 1.20, 1
  UNION ALL SELECT '机械结构工程师', '有限元分析', 1.10, 1
  UNION ALL SELECT '机械结构工程师', '项目管理', 0.70, 0
  UNION ALL SELECT '机械结构工程师', '高分子材料', 0.60, 0

  UNION ALL SELECT '智能产线实施工程师', 'PLC', 1.20, 1
  UNION ALL SELECT '智能产线实施工程师', '工业机器人', 1.20, 1
  UNION ALL SELECT '智能产线实施工程师', '自动控制', 1.00, 1
  UNION ALL SELECT '智能产线实施工程师', '机械设计', 0.80, 0
  UNION ALL SELECT '智能产线实施工程师', '项目管理', 0.70, 0

  UNION ALL SELECT '土木设计工程师', 'BIM', 1.20, 1
  UNION ALL SELECT '土木设计工程师', '工程测量', 1.10, 1
  UNION ALL SELECT '土木设计工程师', 'CAD', 1.00, 1
  UNION ALL SELECT '土木设计工程师', '项目管理', 0.80, 0
  UNION ALL SELECT '土木设计工程师', '安全规范', 0.80, 0

  UNION ALL SELECT '工程项目管理专员', '项目管理', 1.30, 1
  UNION ALL SELECT '工程项目管理专员', 'BIM', 0.90, 0
  UNION ALL SELECT '工程项目管理专员', '成本控制', 1.10, 1
  UNION ALL SELECT '工程项目管理专员', '数据分析', 0.80, 0
  UNION ALL SELECT '工程项目管理专员', '沟通协作', 0.90, 1

  UNION ALL SELECT '金融数据产品经理', '金融建模', 1.10, 1
  UNION ALL SELECT '金融数据产品经理', '数据分析', 1.10, 1
  UNION ALL SELECT '金融数据产品经理', 'SQL', 1.00, 1
  UNION ALL SELECT '金融数据产品经理', '项目管理', 0.90, 0
  UNION ALL SELECT '金融数据产品经理', '风险控制', 1.00, 1

  UNION ALL SELECT '量化风控分析师', 'Python', 1.10, 1
  UNION ALL SELECT '量化风控分析师', '统计建模', 1.20, 1
  UNION ALL SELECT '量化风控分析师', '风险控制', 1.20, 1
  UNION ALL SELECT '量化风控分析师', '金融建模', 1.10, 1
  UNION ALL SELECT '量化风控分析师', 'SQL', 0.90, 0

  UNION ALL SELECT '市场策划专员', '市场分析', 1.20, 1
  UNION ALL SELECT '市场策划专员', '品牌运营', 1.20, 1
  UNION ALL SELECT '市场策划专员', '数据分析', 0.90, 0
  UNION ALL SELECT '市场策划专员', '商务沟通', 1.00, 1
  UNION ALL SELECT '市场策划专员', '项目管理', 0.80, 0

  UNION ALL SELECT '人才发展专员', '人力资源管理', 1.20, 1
  UNION ALL SELECT '人才发展专员', '薪酬绩效', 1.10, 1
  UNION ALL SELECT '人才发展专员', '数据分析', 0.80, 0
  UNION ALL SELECT '人才发展专员', '商务沟通', 1.00, 1
  UNION ALL SELECT '人才发展专员', '项目管理', 0.70, 0

  UNION ALL SELECT '跨境贸易运营专员', '国际贸易', 1.20, 1
  UNION ALL SELECT '跨境贸易运营专员', '商务英语', 1.00, 1
  UNION ALL SELECT '跨境贸易运营专员', '市场分析', 0.90, 0
  UNION ALL SELECT '跨境贸易运营专员', '数据分析', 0.80, 0
  UNION ALL SELECT '跨境贸易运营专员', '项目管理', 0.80, 0

  UNION ALL SELECT '商务英语客户经理', '商务英语', 1.20, 1
  UNION ALL SELECT '商务英语客户经理', '国际贸易', 1.00, 0
  UNION ALL SELECT '商务英语客户经理', '商务沟通', 1.10, 1
  UNION ALL SELECT '商务英语客户经理', '市场分析', 0.80, 0
  UNION ALL SELECT '商务英语客户经理', '项目管理', 0.70, 0

  UNION ALL SELECT '本地化翻译工程师', '翻译技术', 1.30, 1
  UNION ALL SELECT '本地化翻译工程师', 'CATTI规范', 1.10, 1
  UNION ALL SELECT '本地化翻译工程师', '商务英语', 1.00, 1
  UNION ALL SELECT '本地化翻译工程师', '语料处理', 0.90, 0
  UNION ALL SELECT '本地化翻译工程师', '项目管理', 0.70, 0

  UNION ALL SELECT '双语课程教研专员', '教学设计', 1.10, 1
  UNION ALL SELECT '双语课程教研专员', '课程开发', 1.20, 1
  UNION ALL SELECT '双语课程教研专员', '商务英语', 0.90, 0
  UNION ALL SELECT '双语课程教研专员', '教育测评', 0.90, 0
  UNION ALL SELECT '双语课程教研专员', '跨文化沟通', 0.90, 0

  UNION ALL SELECT '化学检测工程师', '化学分析', 1.30, 1
  UNION ALL SELECT '化学检测工程师', '实验设计', 1.00, 1
  UNION ALL SELECT '化学检测工程师', '安全规范', 1.00, 1
  UNION ALL SELECT '化学检测工程师', '数据分析', 0.70, 0
  UNION ALL SELECT '化学检测工程师', 'Python', 0.60, 0

  UNION ALL SELECT '材料研发工程师', '高分子材料', 1.20, 1
  UNION ALL SELECT '材料研发工程师', '化学分析', 1.00, 1
  UNION ALL SELECT '材料研发工程师', '实验设计', 1.00, 1
  UNION ALL SELECT '材料研发工程师', '有限元分析', 0.80, 0
  UNION ALL SELECT '材料研发工程师', '项目管理', 0.70, 0

  UNION ALL SELECT '生物实验技术员', '生物实验', 1.20, 1
  UNION ALL SELECT '生物实验技术员', '分子生物学', 1.20, 1
  UNION ALL SELECT '生物实验技术员', '实验设计', 1.00, 1
  UNION ALL SELECT '生物实验技术员', '数据分析', 0.70, 0
  UNION ALL SELECT '生物实验技术员', '安全规范', 0.70, 0

  UNION ALL SELECT '环境监测工程师', '环境监测', 1.20, 1
  UNION ALL SELECT '环境监测工程师', 'GIS', 1.10, 1
  UNION ALL SELECT '环境监测工程师', '化学分析', 1.00, 1
  UNION ALL SELECT '环境监测工程师', '数据分析', 0.90, 0
  UNION ALL SELECT '环境监测工程师', '项目管理', 0.70, 0

  UNION ALL SELECT '教育技术产品专员', '教学设计', 1.10, 1
  UNION ALL SELECT '教育技术产品专员', '课程开发', 1.10, 1
  UNION ALL SELECT '教育技术产品专员', 'Vue.js', 0.90, 0
  UNION ALL SELECT '教育技术产品专员', '数据分析', 0.90, 0
  UNION ALL SELECT '教育技术产品专员', '教育测评', 0.80, 0

  UNION ALL SELECT '学前教育课程专员', '儿童心理', 1.20, 1
  UNION ALL SELECT '学前教育课程专员', '课程开发', 1.20, 1
  UNION ALL SELECT '学前教育课程专员', '教学设计', 1.00, 1
  UNION ALL SELECT '学前教育课程专员', '教育测评', 0.80, 0
  UNION ALL SELECT '学前教育课程专员', '沟通协作', 0.90, 0

  UNION ALL SELECT '网络安全工程师', '网络安全', 1.30, 1
  UNION ALL SELECT '网络安全工程师', '渗透测试', 1.20, 1
  UNION ALL SELECT '网络安全工程师', 'Linux', 1.00, 1
  UNION ALL SELECT '网络安全工程师', 'Python', 0.80, 0
  UNION ALL SELECT '网络安全工程师', '通信协议', 0.70, 0

  UNION ALL SELECT '医疗信息实施工程师', '临床信息系统', 1.20, 1
  UNION ALL SELECT '医疗信息实施工程师', '项目管理', 1.00, 1
  UNION ALL SELECT '医疗信息实施工程师', 'SQL', 0.90, 1
  UNION ALL SELECT '医疗信息实施工程师', '沟通协作', 0.90, 1
  UNION ALL SELECT '医疗信息实施工程师', '教学设计', 0.60, 0

  UNION ALL SELECT '自动化控制工程师', '自动控制', 1.30, 1
  UNION ALL SELECT '自动化控制工程师', 'PLC', 1.20, 1
  UNION ALL SELECT '自动化控制工程师', '工业机器人', 1.00, 0
  UNION ALL SELECT '自动化控制工程师', 'Python', 0.80, 0
  UNION ALL SELECT '自动化控制工程师', '嵌入式开发', 0.80, 0

  UNION ALL SELECT '工业机器视觉工程师', 'Python', 1.10, 1
  UNION ALL SELECT '工业机器视觉工程师', '机器学习', 1.10, 1
  UNION ALL SELECT '工业机器视觉工程师', '工业机器人', 0.90, 0
  UNION ALL SELECT '工业机器视觉工程师', '自动控制', 0.90, 0
  UNION ALL SELECT '工业机器视觉工程师', 'C++', 1.00, 1
) x ON x.job_title = j.title AND x.skill_name = s.name;

-- 学生样本：每专业 1 人
INSERT INTO students (
  student_no, full_name, gender, college_id, major_id, graduation_year,
  email, phone, resume_text, technical_projects_text, capability_summary_text
)
SELECT x.student_no,
       x.full_name,
       x.gender,
       m.college_id,
       m.id,
       x.graduation_year,
       x.email,
       x.phone,
       x.resume_text,
       x.technical_projects_text,
       x.capability_summary_text
FROM majors m
JOIN (
  SELECT 'S2026001' AS student_no, '张晨曦' AS full_name, 'male' AS gender, 'M001' AS major_code, 2026 AS graduation_year,
         'zhangcx@example.com' AS email, '13800010001' AS phone,
         '熟悉 Java 后端开发、SQL 与 Linux 环境，参与过校园服务系统开发。' AS resume_text,
         '完成校园报修平台后端 API 设计与数据库建模。' AS technical_projects_text,
         '具备需求拆解、团队协作和持续迭代能力。' AS capability_summary_text
  UNION ALL SELECT 'S2026002', '李思远', 'male', 'M002', 2026, 'lisy@example.com', '13800010002',
         '掌握 C++、Java 与基础网络协议，具备计算机系统基础。',
         '完成高并发课程选课系统性能优化实验。',
         '具备系统分析、问题定位与代码优化能力。'
  UNION ALL SELECT 'S2026003', '王浩然', 'male', 'M003', 2026, 'wanghr@example.com', '13800010003',
         '熟悉网络安全基础、Linux 运维与脚本自动化。',
         '完成校园网日志审计与告警脚本开发项目。',
         '具备安全意识、应急处理与故障排查能力。'
  UNION ALL SELECT 'S2026004', '陈雨桐', 'female', 'M004', 2026, 'chenyt@example.com', '13800010004',
         '熟悉 Python、SQL、机器学习与数据分析流程。',
         '完成用户流失预测模型与可视化看板。',
         '具备数据洞察、模型复盘与业务沟通能力。'
  UNION ALL SELECT 'S2026005', '赵文博', 'male', 'M005', 2026, 'zhaowb@example.com', '13800010005',
         '掌握统计建模、Python 与风险分析方法。',
         '完成信贷样本违约概率统计分析实践。',
         '具备严谨分析、建模解释与报告写作能力。'
  UNION ALL SELECT 'S2026006', '刘星辰', 'male', 'M006', 2026, 'liuxc@example.com', '13800010006',
         '掌握信号处理、嵌入式开发与通信基础。',
         '完成通信信号采集与滤波算法实验项目。',
         '具备软硬件协同调试与工程实现能力。'
  UNION ALL SELECT 'S2026007', '黄子涵', 'female', 'M007', 2026, 'huangzh@example.com', '13800010007',
         '熟悉通信协议、网络优化指标与 Python 脚本。',
         '完成小区网络参数优化仿真项目。',
         '具备指标分析、方案执行与复盘能力。'
  UNION ALL SELECT 'S2026008', '周景明', 'male', 'M008', 2026, 'zhoujm@example.com', '13800010008',
         '掌握机械设计、CAD 与有限元分析。',
         '完成机械臂支架结构设计与应力仿真。',
         '具备结构设计、工程沟通与迭代优化能力。'
  UNION ALL SELECT 'S2026009', '吴若溪', 'female', 'M009', 2026, 'wurx@example.com', '13800010009',
         '掌握 PLC、工业机器人基础与产线调试方法。',
         '完成机器人分拣产线联调课程设计。',
         '具备系统集成、现场协同与问题闭环能力。'
  UNION ALL SELECT 'S2026010', '徐嘉伟', 'male', 'M010', 2026, 'xujw@example.com', '13800010010',
         '熟悉自动控制、PLC 与工业控制网络。',
         '完成自动化立库控制逻辑优化项目。',
         '具备控制策略设计与现场调试能力。'
  UNION ALL SELECT 'S2026011', '孙可欣', 'female', 'M011', 2026, 'sunkx@example.com', '13800010011',
         '掌握 BIM、工程测量与土木设计基础。',
         '完成校园道路改造工程建模与测量实践。',
         '具备工程实施、质量意识与协作能力。'
  UNION ALL SELECT 'S2026012', '马俊杰', 'male', 'M012', 2026, 'majj@example.com', '13800010012',
         '熟悉工程项目管理、成本控制与进度计划。',
         '完成教学楼改造项目管理模拟实训。',
         '具备统筹协调、风险识别与沟通能力。'
  UNION ALL SELECT 'S2026013', '朱梦瑶', 'female', 'M013', 2026, 'zhumy@example.com', '13800010013',
         '掌握市场分析、品牌运营和活动策划基础。',
         '完成校园品牌传播与用户调研项目。',
         '具备创意表达、数据复盘与执行落地能力。'
  UNION ALL SELECT 'S2026014', '胡嘉宁', 'female', 'M014', 2026, 'hujn@example.com', '13800010014',
         '熟悉人力资源管理、绩效分析与员工沟通。',
         '完成企业实习招聘流程优化方案设计。',
         '具备组织协调、制度执行与服务意识。'
  UNION ALL SELECT 'S2026015', '郭子衡', 'male', 'M015', 2026, 'guozh@example.com', '13800010015',
         '掌握金融建模、风险控制与数据分析方法。',
         '完成基金组合风险评估与策略回测项目。',
         '具备量化思维、风险意识与结果解释能力。'
  UNION ALL SELECT 'S2026016', '何晓彤', 'female', 'M016', 2026, 'hext@example.com', '13800010016',
         '熟悉国际贸易流程、商务英语与跨境业务协同。',
         '完成跨境订单履约流程优化分析项目。',
         '具备跨文化沟通、执行推进与问题处理能力。'
  UNION ALL SELECT 'S2026017', '高嘉怡', 'female', 'M017', 2026, 'gaojy@example.com', '13800010017',
         '具备商务英语沟通、外语写作与跨文化理解能力。',
         '完成英文客户沟通与服务脚本设计项目。',
         '具备表达清晰、沟通协同与学习迁移能力。'
  UNION ALL SELECT 'S2026018', '林语菲', 'female', 'M018', 2026, 'linyf@example.com', '13800010018',
         '掌握翻译技术、术语管理与语料处理基础。',
         '完成教育内容本地化翻译与审校实践项目。',
         '具备语言转换、规范意识与交付质量能力。'
  UNION ALL SELECT 'S2026019', '罗晨熙', 'male', 'M019', 2026, 'luocx@example.com', '13800010019',
         '掌握化学分析、实验设计与实验室安全规范。',
         '完成材料样品成分检测与数据记录实践。',
         '具备实验严谨性、数据整理与问题追踪能力。'
  UNION ALL SELECT 'S2026020', '郑一鸣', 'male', 'M020', 2026, 'zhengym@example.com', '13800010020',
         '熟悉高分子材料基础、材料实验与性能分析。',
         '完成复合材料性能对比实验项目。',
         '具备研发思维、实验复盘与工程协同能力。'
  UNION ALL SELECT 'S2026021', '梁可人', 'female', 'M021', 2026, 'liangkr@example.com', '13800010021',
         '掌握生物实验操作、分子检测与实验记录。',
         '完成细胞样本检测流程优化项目。',
         '具备实验规范、数据整理与团队协作能力。'
  UNION ALL SELECT 'S2026022', '谢安然', 'female', 'M022', 2026, 'xiear@example.com', '13800010022',
         '熟悉环境监测、GIS 与环境数据分析方法。',
         '完成园区空气质量监测数据分析项目。',
         '具备现场执行、监测报告与沟通表达能力。'
  UNION ALL SELECT 'S2026023', '宋知远', 'male', 'M023', 2026, 'songzy@example.com', '13800010023',
         '掌握教学设计、课程开发与学习数据分析。',
         '完成课堂互动工具原型与教学反馈分析。',
         '具备教育产品思维、用户沟通与迭代能力。'
  UNION ALL SELECT 'S2026024', '唐芷若', 'female', 'M024', 2026, 'tangzr@example.com', '13800010024',
         '熟悉儿童心理与学前课程活动设计。',
         '完成幼儿主题活动课程包设计与试讲。',
         '具备儿童观察、课程实施与家园沟通能力。'
) x ON x.major_code = m.code;

-- 就业样本：每专业 1 条就业记录
INSERT INTO employment_records (
  student_id, company_id, job_id, employment_type, employment_status,
  start_date, end_date, evidence_source, notes
)
SELECT s.id,
       c.id,
       j.id,
       x.employment_type,
       x.employment_status,
       x.start_date,
       NULL,
       x.evidence_source,
       x.notes
FROM students s
JOIN (
  SELECT 'S2026001' AS student_no, '华云科技股份有限公司' AS company_name, '后端开发工程师' AS job_title,
         'signed' AS employment_type, 'active' AS employment_status, '2026-07-01' AS start_date,
         '企业签约回执' AS evidence_source, '与专业高度对口' AS notes
  UNION ALL SELECT 'S2026002', '华云科技股份有限公司', '平台运维工程师', 'signed', 'active', '2026-07-05', '企业签约回执', '系统运维方向就业'
  UNION ALL SELECT 'S2026003', '安澜网络安全有限公司', '网络安全工程师', 'signed', 'active', '2026-07-10', '企业签约回执', '网络安全方向就业'
  UNION ALL SELECT 'S2026004', '智源数据服务有限公司', '数据分析工程师', 'signed', 'active', '2026-07-03', '企业签约回执', '数据分析岗位'
  UNION ALL SELECT 'S2026005', '博瑞金融科技有限公司', '量化风控分析师', 'offer', 'active', '2026-07-08', '录用通知书', '统计与风控结合岗位'
  UNION ALL SELECT 'S2026006', '星链通信技术有限公司', '5G通信算法工程师', 'signed', 'active', '2026-07-12', '企业签约回执', '通信算法岗位'
  UNION ALL SELECT 'S2026007', '星链通信技术有限公司', '网络优化工程师', 'signed', 'active', '2026-07-15', '企业签约回执', '通信网优岗位'
  UNION ALL SELECT 'S2026008', '远航智能装备有限公司', '机械结构工程师', 'signed', 'active', '2026-07-02', '企业签约回执', '结构设计岗位'
  UNION ALL SELECT 'S2026009', '远航智能装备有限公司', '智能产线实施工程师', 'signed', 'active', '2026-07-04', '企业签约回执', '产线实施岗位'
  UNION ALL SELECT 'S2026010', '天枢工业自动化有限公司', '自动化控制工程师', 'signed', 'active', '2026-07-06', '企业签约回执', '自动化控制岗位'
  UNION ALL SELECT 'S2026011', '城建数智工程集团', '土木设计工程师', 'signed', 'active', '2026-07-01', '企业签约回执', '土木设计岗位'
  UNION ALL SELECT 'S2026012', '城建数智工程集团', '工程项目管理专员', 'signed', 'active', '2026-07-09', '企业签约回执', '工程管理岗位'
  UNION ALL SELECT 'S2026013', '海纳市场咨询有限公司', '市场策划专员', 'signed', 'active', '2026-07-11', '企业签约回执', '营销策划岗位'
  UNION ALL SELECT 'S2026014', '海纳市场咨询有限公司', '人才发展专员', 'signed', 'active', '2026-07-13', '企业签约回执', '人力方向岗位'
  UNION ALL SELECT 'S2026015', '博瑞金融科技有限公司', '金融数据产品经理', 'signed', 'active', '2026-07-05', '企业签约回执', '金融科技岗位'
  UNION ALL SELECT 'S2026016', '纵横物流科技有限公司', '跨境贸易运营专员', 'signed', 'active', '2026-07-07', '企业签约回执', '跨境贸易岗位'
  UNION ALL SELECT 'S2026017', '纵横物流科技有限公司', '商务英语客户经理', 'offer', 'active', '2026-07-15', '录用通知书', '外语商务岗位'
  UNION ALL SELECT 'S2026018', '新语教育科技有限公司', '本地化翻译工程师', 'signed', 'active', '2026-07-14', '企业签约回执', '翻译本地化岗位'
  UNION ALL SELECT 'S2026019', '晶材新材料有限公司', '化学检测工程师', 'signed', 'active', '2026-07-16', '企业签约回执', '化学检测岗位'
  UNION ALL SELECT 'S2026020', '晶材新材料有限公司', '材料研发工程师', 'signed', 'active', '2026-07-18', '企业签约回执', '材料研发岗位'
  UNION ALL SELECT 'S2026021', '绿洲生物医药有限公司', '生物实验技术员', 'signed', 'active', '2026-07-20', '企业签约回执', '生物实验岗位'
  UNION ALL SELECT 'S2026022', '清源环保科技有限公司', '环境监测工程师', 'signed', 'active', '2026-07-22', '企业签约回执', '环境监测岗位'
  UNION ALL SELECT 'S2026023', '新语教育科技有限公司', '教育技术产品专员', 'signed', 'active', '2026-07-24', '企业签约回执', '教育技术岗位'
  UNION ALL SELECT 'S2026024', '青禾幼教发展集团', '学前教育课程专员', 'signed', 'active', '2026-07-26', '企业签约回执', '学前课程岗位'
) x ON x.student_no = s.student_no
JOIN companies c ON c.name = x.company_name
JOIN jobs j ON j.title = x.job_title AND j.company_id = c.id;

-- 匹配结果样本：用于看板展示对口率与技能缺口
INSERT INTO match_results (
  student_id, major_id, job_id, algorithm_version,
  semantic_score, skill_score, constraint_score, final_score,
  matched_skills_json, missing_skills_json, constraint_failures_json
)
SELECT s.id,
       s.major_id,
       j.id,
       'v0.1-baseline',
       x.semantic_score,
       x.skill_score,
       x.constraint_score,
       x.final_score,
       JSON_ARRAY(JSON_OBJECT('skill_name', x.matched_skill, 'job_weight', 1.0, 'major_weight', 1.0)),
       JSON_ARRAY(JSON_OBJECT('skill_name', x.missing_skill, 'mandatory', x.missing_mandatory)),
       CASE
         WHEN x.constraint_type = '' THEN JSON_ARRAY()
         ELSE JSON_ARRAY(JSON_OBJECT('type', x.constraint_type, 'message', x.constraint_message))
       END
FROM students s
JOIN (
  SELECT 'S2026001' AS student_no, '后端开发工程师' AS job_title,
         88.0 AS semantic_score, 84.0 AS skill_score, 1.0 AS constraint_score, 86.2 AS final_score,
         'Java' AS matched_skill, 'Kubernetes' AS missing_skill, 0 AS missing_mandatory,
         '' AS constraint_type, '' AS constraint_message
  UNION ALL SELECT 'S2026002', '平台运维工程师', 78.0, 72.0, 1.0, 75.3, 'Linux', 'Kubernetes', 1, '', ''
  UNION ALL SELECT 'S2026003', '网络安全工程师', 70.0, 65.0, 0.95, 64.1, '网络安全', '渗透测试', 1, 'location_city', 'preferred city does not match job city'
  UNION ALL SELECT 'S2026004', '数据分析工程师', 86.0, 80.0, 1.0, 83.3, '数据分析', '深度学习', 0, '', ''
  UNION ALL SELECT 'S2026005', '量化风控分析师', 72.0, 66.0, 1.0, 69.3, '统计建模', '金融建模', 1, '', ''
  UNION ALL SELECT 'S2026006', '5G通信算法工程师', 81.0, 74.0, 1.0, 77.9, '通信协议', 'Python', 0, '', ''
  UNION ALL SELECT 'S2026007', '网络优化工程师', 74.0, 69.0, 1.0, 71.8, '网络优化', '数据分析', 0, '', ''
  UNION ALL SELECT 'S2026008', '机械结构工程师', 75.0, 70.0, 1.0, 72.8, '机械设计', '工业机器人', 1, '', ''
  UNION ALL SELECT 'S2026009', '智能产线实施工程师', 88.0, 83.0, 1.0, 85.8, 'PLC', 'Python', 0, '', ''
  UNION ALL SELECT 'S2026010', '自动化控制工程师', 82.0, 76.0, 1.0, 79.3, '自动控制', '嵌入式开发', 1, '', ''
  UNION ALL SELECT 'S2026011', '土木设计工程师', 72.0, 66.0, 1.0, 69.3, 'BIM', '项目管理', 1, '', ''
  UNION ALL SELECT 'S2026012', '工程项目管理专员', 80.0, 74.0, 1.0, 77.3, '项目管理', '成本控制', 1, '', ''
  UNION ALL SELECT 'S2026013', '市场策划专员', 79.0, 73.0, 1.0, 76.3, '市场分析', '商务沟通', 1, '', ''
  UNION ALL SELECT 'S2026014', '人才发展专员', 77.0, 71.0, 1.0, 74.3, '人力资源管理', '商务沟通', 1, '', ''
  UNION ALL SELECT 'S2026015', '金融数据产品经理', 84.0, 79.0, 1.0, 81.8, '金融建模', 'SQL', 0, '', ''
  UNION ALL SELECT 'S2026016', '跨境贸易运营专员', 82.0, 76.0, 1.0, 79.3, '国际贸易', '项目管理', 0, '', ''
  UNION ALL SELECT 'S2026017', '商务英语客户经理', 68.0, 63.0, 1.0, 65.8, '商务英语', '市场分析', 0, '', ''
  UNION ALL SELECT 'S2026018', '本地化翻译工程师', 86.0, 82.0, 1.0, 84.2, '翻译技术', '语料处理', 1, '', ''
  UNION ALL SELECT 'S2026019', '化学检测工程师', 79.0, 73.0, 1.0, 76.3, '化学分析', 'Python', 0, '', ''
  UNION ALL SELECT 'S2026020', '材料研发工程师', 83.0, 77.0, 1.0, 80.3, '高分子材料', '有限元分析', 0, '', ''
  UNION ALL SELECT 'S2026021', '生物实验技术员', 70.0, 64.0, 1.0, 67.3, '生物实验', '数据分析', 1, '', ''
  UNION ALL SELECT 'S2026022', '环境监测工程师', 85.0, 80.0, 1.0, 82.8, '环境监测', 'GIS', 0, '', ''
  UNION ALL SELECT 'S2026023', '教育技术产品专员', 69.0, 62.0, 1.0, 65.9, '教学设计', 'Vue.js', 1, '', ''
  UNION ALL SELECT 'S2026024', '学前教育课程专员', 90.0, 86.0, 1.0, 88.2, '儿童心理', '教育测评', 0, '', ''
) x ON x.student_no = s.student_no
JOIN jobs j ON j.title = x.job_title;

-- 结束标记
SELECT 'seed_full.sql finished' AS message;
