USE graduate_match;
SET NAMES utf8mb4;

-- =========================================================
-- 答辩展示数据包 v1
-- 目标规模：10 学院 / 20 专业 / 100 学生（每专业 5 人）
-- 用途：匹配分析算法合理性展示（含对口样本与跨域样本）
-- 执行策略：追加导入、幂等更新，不清空现有数据
-- =========================================================

-- 统一密码：Demo@123
-- sha256(Demo@123) = ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73

INSERT INTO users(username, email, password_hash, role, is_active)
VALUES
  ('def_admin', 'def_admin@test.local', 'sha256:ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73', 'admin', 1)
ON DUPLICATE KEY UPDATE
  email = VALUES(email),
  password_hash = VALUES(password_hash),
  role = VALUES(role),
  is_active = VALUES(is_active);

DROP TEMPORARY TABLE IF EXISTS tmp_def_college_seed;
CREATE TEMPORARY TABLE tmp_def_college_seed (
  code VARCHAR(32) PRIMARY KEY,
  name VARCHAR(128) NOT NULL,
  description TEXT NOT NULL
) ENGINE=InnoDB;

INSERT INTO tmp_def_college_seed(code, name, description) VALUES
('DC01', '计算机与智能工程学院（答辩）', '面向软件系统、信息安全与工程化交付能力培养，强调从问题建模到系统落地的全链路训练。'),
('DC02', '电子与通信工程学院（答辩）', '面向通信网络与电子系统研发，突出协议分析、硬件理解与工程优化能力。'),
('DC03', '机械与车辆工程学院（答辩）', '面向智能制造与车辆电子，注重结构设计、控制验证与整机协同。'),
('DC04', '自动化与机器人学院（答辩）', '面向工业控制与机器人系统，强调算法、控制、平台部署一体化能力。'),
('DC05', '材料与化工学院（答辩）', '面向新材料研发与工艺优化，突出实验分析与工程转化能力。'),
('DC06', '生物与医药工程学院（答辩）', '面向生物工艺与药品工程，强调规范意识与质量控制能力。'),
('DC07', '土木与环境工程学院（答辩）', '面向基础设施与绿色治理，强调工程实施与环境合规能力。'),
('DC08', '经济与管理学院（答辩）', '面向市场运营与供应链协同，强调数据驱动决策和业务执行能力。'),
('DC09', '外语与数字媒体学院（答辩）', '面向国际沟通与数字内容生产，强调跨文化表达与技术协作。'),
('DC10', '数学与数据科学学院（答辩）', '面向统计建模与数据智能，强调方法严谨性与业务解释能力。');

INSERT INTO universities(code, name, province, city, description)
VALUES ('DU_SHOW', '毕业设计答辩展示大学', '北京市', '北京', '用于毕业设计答辩展示、算法合理性验证与系统流程演示的专用数据学校。')
ON DUPLICATE KEY UPDATE
  name = VALUES(name),
  province = VALUES(province),
  city = VALUES(city),
  description = VALUES(description);

INSERT INTO colleges(university_id, code, name, description)
SELECT u.id, s.code, s.name, s.description
FROM tmp_def_college_seed s
JOIN universities u ON u.code = 'DU_SHOW'
ON DUPLICATE KEY UPDATE
  university_id = VALUES(university_id),
  description = VALUES(description);

DROP TEMPORARY TABLE IF EXISTS tmp_def_major_seed;
CREATE TEMPORARY TABLE tmp_def_major_seed (
  major_code VARCHAR(32) PRIMARY KEY,
  college_code VARCHAR(32) NOT NULL,
  major_name VARCHAR(128) NOT NULL,
  discipline VARCHAR(64) NOT NULL,
  orientation TEXT NOT NULL,
  core_courses TEXT NOT NULL,
  knowledge_skills TEXT NOT NULL,
  technical_platform TEXT NOT NULL,
  capability TEXT NOT NULL,
  industry_targets TEXT NOT NULL
) ENGINE=InnoDB;

INSERT INTO tmp_def_major_seed VALUES
('DM001', 'DC01', '软件工程（答辩）', '工学', '面向企业级软件系统分析、后端架构设计、持续集成与高并发服务治理', '数据结构,操作系统,数据库系统,软件工程,计算机网络,软件测试', '需求分析,系统设计,接口开发,代码重构,性能优化,测试验证', 'Linux,Git,MySQL,Docker,C++,Java', '工程协作,问题分解,文档规范,质量意识,持续改进', '互联网平台,工业软件,企业信息化'),
('DM002', 'DC01', '网络空间安全（答辩）', '工学', '面向安全攻防、漏洞分析、系统加固与安全运营响应', '密码学基础,网络安全,操作系统安全,渗透测试,安全审计', '威胁建模,漏洞复现,日志分析,安全策略配置,应急响应', 'Linux,Python,Wireshark,SIEM,安全扫描工具', '风险意识,证据链思维,合规意识,快速响应,团队协同', '政企安全,金融安全,云平台安全'),
('DM003', 'DC02', '通信工程（答辩）', '工学', '面向移动通信网络规划、优化与通信质量保障', '通信原理,移动通信,信号与系统,网络规划,协议分析', '网络优化,故障定位,链路分析,指标监控,参数调优', 'Linux,抓包工具,优化软件,Python', '现场排障,数据解读,稳定性保障,沟通协作,文档复盘', '运营商网络,工业通信,智慧城市'),
('DM004', 'DC02', '电子信息工程（答辩）', '工学', '面向电子系统设计、嵌入式开发与硬件联调验证', '电路分析,数字电子技术,模拟电子技术,嵌入式系统,信号处理', '硬件调试,驱动开发,接口联调,系统验证,问题复现', 'C,示波器,EDA,嵌入式开发板,CAN工具', '实验严谨,工程验证,跨端协作,质量追踪,持续优化', '车载电子,智能终端,工业控制'),
('DM005', 'DC03', '车辆工程（答辩）', '工学', '面向整车电子架构、车载控制系统与工程验证', '汽车构造,车载电子,控制理论,CAN总线,整车测试', '控制策略分析,车载系统联调,故障诊断,测试闭环', 'C,CANoe,Matlab,测试台架', '工程执行,安全意识,过程管理,团队沟通,问题闭环', '智能汽车,新能源整车,汽车供应链'),
('DM006', 'DC03', '机械设计制造及其自动化（答辩）', '工学', '面向机械结构设计、制造工艺优化与自动化产线协同', '机械设计,机械制造技术,控制基础,工程制图,机电一体化', '结构设计,工艺分析,设备调试,精度控制,成本优化', 'SolidWorks,AutoCAD,PLC,Matlab', '工程规范,现场执行,跨部门协作,质量控制,持续改进', '装备制造,汽车制造,智能工厂'),
('DM007', 'DC04', '自动化（答辩）', '工学', '面向工业控制系统设计、PLC编程与控制策略实施', '自动控制原理,现代控制理论,PLC技术,传感器技术,工业网络', '控制建模,参数整定,系统联调,故障诊断,稳定性优化', 'PLC,组态软件,Matlab,工业总线工具', '逻辑严谨,系统思维,风险识别,执行落地,复盘改进', '智能制造,流程工业,产线自动化'),
('DM008', 'DC04', '机器人工程（答辩）', '工学', '面向机器人感知、路径规划、控制算法与场景部署', '机器人学,机器视觉,运动控制,嵌入式系统,人工智能基础', '路径规划,视觉识别,算法调优,系统集成,场景验证', 'ROS,Python,C++,OpenCV,Linux', '系统集成,迭代优化,跨域协作,工程交付,故障定位', '工业机器人,服务机器人,仓储物流'),
('DM009', 'DC05', '材料科学与工程（答辩）', '工学', '面向先进材料研发、性能评估与应用转化', '材料科学基础,材料分析方法,材料制备工艺,物理化学', '实验设计,性能测试,数据分析,工艺改进,质量评估', '材料测试设备,统计软件,实验管理平台', '科研规范,实验复现,数据严谨,过程管理,协同创新', '新能源材料,电子材料,高端制造'),
('DM010', 'DC05', '应用化学（答辩）', '理学', '面向化学工艺开发、分析检测与生产质量控制', '有机化学,分析化学,物理化学,化工原理,仪器分析', '化学分析,工艺优化,实验安全,质量控制,问题溯源', '色谱仪,光谱仪,实验室信息系统,数据分析工具', '安全意识,规范执行,实验管理,结果复盘,持续改进', '精细化工,环保检测,新材料工艺'),
('DM011', 'DC06', '生物工程（答辩）', '工学', '面向生物工艺开发、发酵流程优化与数据化管理', '生物化学,微生物学,生物反应工程,生物分离工程,实验设计', '工艺设计,参数优化,实验记录,数据分析,流程验证', '生物反应器,实验室设备,统计软件,质量系统', '规范意识,实验严谨,过程追踪,协同沟通,问题定位', '生物制造,医药生产,食品工程'),
('DM012', 'DC06', '制药工程（答辩）', '工学', '面向药品研发生产、GMP体系执行与质量管理', '药剂学,药物分析,制药工程学,药品质量管理,GMP规范', '工艺执行,质量检验,偏差分析,验证管理,文档合规', '质量管理系统,实验仪器,统计分析软件', '合规意识,质量底线,流程执行,文档管理,持续优化', '创新药,仿制药,医药制造'),
('DM013', 'DC07', '土木工程（答辩）', '工学', '面向土木结构设计、BIM协同与工程实施管理', '结构力学,混凝土结构,工程测量,施工组织,BIM技术', '结构分析,施工管理,质量控制,进度跟踪,风险识别', 'BIM平台,工程管理软件,测量仪器,AutoCAD', '现场管理,工程组织,质量意识,沟通协调,成本控制', '基础设施,建筑工程,市政工程'),
('DM014', 'DC07', '环境工程（答辩）', '工学', '面向污染治理、环境监测与绿色工程实施', '环境化学,水污染控制,大气污染控制,环境监测,环境影响评价', '监测分析,治理方案设计,数据评估,合规报告,项目实施', '监测设备,GIS,数据分析软件,环保信息平台', '社会责任,合规意识,问题分析,方案表达,协作执行', '环保治理,工业减排,生态修复'),
('DM015', 'DC08', '市场营销（答辩）', '管理学', '面向市场洞察、活动运营、品牌建设与增长策略执行', '市场调研,消费者行为,数字营销,品牌管理,运营分析', '活动策划,用户增长,数据复盘,内容运营,渠道协同', 'Excel,BI,CRM,内容平台', '沟通表达,组织协调,执行推进,目标管理,复盘优化', '零售电商,消费品牌,互联网运营'),
('DM016', 'DC08', '物流管理（答辩）', '管理学', '面向供应链协同、库存优化与物流系统规划', '物流学,供应链管理,仓储管理,运输管理,运筹学基础', '计划编制,库存分析,流程优化,跨部门协同,异常处理', 'WMS,TMS,ERP,数据分析工具', '系统思维,协同能力,执行力,数据意识,风险管理', '制造物流,电商物流,冷链物流'),
('DM017', 'DC09', '商务英语（答辩）', '文学', '面向国际商务沟通、外贸协同与跨文化项目支持', '商务英语,国际贸易实务,跨文化沟通,商务写作,口译基础', '英文沟通,商务谈判支持,文档处理,客户协同,信息整理', '办公软件,翻译辅助工具,协同平台', '跨文化理解,表达能力,服务意识,流程执行,团队协作', '国际贸易,跨境电商,外企服务'),
('DM018', 'DC09', '数字媒体技术（答辩）', '工学', '面向数字内容生产、交互开发与多媒体技术实现', '数字图像处理,多媒体技术,前端开发基础,交互设计,视频制作', '内容制作,交互实现,素材管理,项目协作,效果优化', 'Premiere,AfterEffects,HTML/CSS/JS,图形工具', '创意表达,需求理解,协同执行,用户视角,快速迭代', '新媒体内容,数字营销,文创科技'),
('DM019', 'DC10', '应用统计学（答辩）', '理学', '面向统计建模、实验设计与业务数据解释', '概率论,数理统计,回归分析,时间序列,抽样调查', '统计建模,实验设计,结果解释,指标构建,不确定性分析', 'R,Python,SQL,可视化工具', '逻辑严谨,证据导向,模型解释,沟通表达,持续学习', '金融分析,运营分析,风险评估'),
('DM020', 'DC10', '数据科学与大数据技术（答辩）', '工学', '面向数据治理、机器学习建模与数据产品化实践', '机器学习,数据仓库,大数据处理,数据库系统,数据可视化', '数据清洗,特征工程,模型训练,效果评估,业务落地', 'Python,Spark,SQL,Docker,BI平台', '工程实践,数据思维,问题抽象,团队协作,价值表达', '智能制造,互联网分析,商业智能');

INSERT INTO majors(college_id, code, name, description)
SELECT c.id,
       s.major_code,
       s.major_name,
       CONCAT(
         '学科门类：', s.discipline, '。本专业围绕', s.industry_targets,
         '等行业需求设置培养目标，强调“知识体系、工程实践、职业素养”三位一体培养。',
         '培养方案采用“通识基础—学科基础—方向核心—综合实践—企业实习”五段式结构，核心课程覆盖',
         s.core_courses,
         '。在课程学习与项目实践过程中，学生需系统掌握', s.knowledge_skills,
         '，熟练使用', s.technical_platform,
         '，并通过课程设计、企业课题、毕业设计形成完整工程闭环。',
         '最终要求学生具备', s.capability,
         '等综合能力，能够在真实业务场景中完成方案设计、实施落地、评估优化和持续迭代。'
       ) AS description
FROM tmp_def_major_seed s
JOIN colleges c ON c.code = s.college_code
ON DUPLICATE KEY UPDATE
  code = VALUES(code),
  description = VALUES(description);

INSERT INTO major_profiles(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text)
SELECT m.id,
       CONCAT('核心课程体系：', s.core_courses, '。课程群按照“基础知识夯实、专业能力进阶、综合项目训练、企业实践验证”的路径组织，要求学生在每一阶段完成可量化成果，确保知识点与岗位需求形成映射关系。课程考核采用“过程评价+项目答辩+文档审查”的组合方式，突出学习过程可追踪、学习结果可解释、能力成长可复盘。'),
       CONCAT('专业知识与技能：', s.knowledge_skills, '。强调在真实问题中进行需求拆解、方案比较、实现验证和结果复盘，训练学生在复杂约束下完成高质量交付的能力。通过案例演练与跨课程综合任务，学生需要形成从理论理解到工程落地的连续能力链条，能够独立分析问题并提出可执行改进方案。并要求在学期总结中完成技能证据归档，保证能力成长路径清晰可审阅。'),
       CONCAT('技术平台要求：', s.technical_platform, '。通过实验平台、课程项目与实习场景反复训练，确保学生能够把工具能力转化为工程产出，而不是停留在概念层面。平台训练强调环境配置、工具协同、版本管理、质量保障和交付复盘，要求学生输出可运行成果与结构化技术说明。同时要求学生在团队环境中执行统一规范，确保协作开发过程可追踪、可复现、可维护。'),
       CONCAT('能力培养目标：', s.capability, '。最终形成“问题发现—问题分析—方案设计—执行落地—效果评估—持续优化”的完整能力闭环，用于支撑毕业后岗位适配与职业发展。培养过程强调职业规范、协作意识与责任意识，要求学生具备在多角色团队中稳定交付并持续进化的综合素养，并能够在实际岗位中快速完成角色适应与价值创造。')
FROM tmp_def_major_seed s
JOIN colleges c ON c.code = s.college_code
JOIN majors m ON m.college_id = c.id AND m.name = s.major_name
ON DUPLICATE KEY UPDATE
  core_courses_text = VALUES(core_courses_text),
  knowledge_skills_text = VALUES(knowledge_skills_text),
  technical_platform_text = VALUES(technical_platform_text),
  capability_text = VALUES(capability_text);

DROP TEMPORARY TABLE IF EXISTS tmp_def_company_seed;
CREATE TEMPORARY TABLE tmp_def_company_seed (
  company_code VARCHAR(16) PRIMARY KEY,
  name VARCHAR(255) NOT NULL,
  domain_focus VARCHAR(128) NOT NULL,
  province VARCHAR(64) NOT NULL,
  city VARCHAR(64) NOT NULL,
  industry_scene VARCHAR(255) NOT NULL
) ENGINE=InnoDB;

INSERT INTO tmp_def_company_seed VALUES
('C01', '星澜软件科技（答辩）', '企业级软件平台与工业数字化', '北京市', '北京', '工业互联网,企业信息化,智能制造'),
('C02', '华讯网络系统（答辩）', '通信网络优化与电子系统集成', '上海市', '上海', '运营商网络,智慧园区,工业通信'),
('C03', '智行汽车电子（答辩）', '车载电子与智能汽车控制', '安徽省', '合肥', '智能汽车,新能源汽车,车联网'),
('C04', '灵虎机器人技术（答辩）', '工业机器人与自动化解决方案', '江苏省', '苏州', '智能工厂,仓储物流,装备制造'),
('C05', '新材智造科技（答辩）', '先进材料研发与化工工艺优化', '浙江省', '宁波', '新材料,精细化工,电子制造'),
('C06', '绿源生物医药（答辩）', '生物工艺与药品工程技术', '湖北省', '武汉', '生物制造,医药生产,健康产业'),
('C07', '建安绿色工程（答辩）', '土木建设与环境治理工程', '四川省', '成都', '基础设施,环保治理,市政工程'),
('C08', '云策商业管理（答辩）', '市场运营与供应链协同管理', '广东省', '广州', '零售电商,品牌运营,仓储物流'),
('C09', '视界内容科技（答辩）', '国际商务服务与数字内容技术', '福建省', '厦门', '跨境服务,数字内容,新媒体运营'),
('C10', '数衡数据科技（答辩）', '统计建模与数据智能平台', '浙江省', '杭州', '商业智能,数据分析,风险管理');

INSERT INTO companies(name, main_business, flagship_products, domain_area, application_industries, province, city, website, description)
SELECT s.name,
       CONCAT('企业主业说明：公司长期深耕', s.domain_focus, '方向，围绕客户真实业务场景提供从需求梳理、方案架构、系统开发、实施交付到持续运维的全流程服务。团队采用数据驱动与工程治理双轮机制，强调业务价值可量化、系统质量可追踪、项目风险可闭环，能够支撑多行业复杂场景持续迭代。'),
       CONCAT('拳头产品说明：公司核心产品聚焦', s.domain_focus, '，具备模块化架构、可扩展接口和稳定部署能力，支持多角色协同与跨系统集成。产品通过持续版本演进覆盖“监控预警、业务分析、流程编排、决策辅助”等关键能力，已在', s.industry_scene, '等场景形成可复制落地方案。'),
       CONCAT('业务领域：', s.domain_focus, '。公司在该领域通过“平台能力+行业方案”模式构建竞争优势，重视技术沉淀与业务理解的协同发展。'),
       CONCAT('应用行业：', s.industry_scene, '。行业覆盖广、项目类型多，能够为毕业生提供从基础执行到方案优化的成长路径。'),
       s.province,
       s.city,
       CONCAT('https://example.com/defense/', LOWER(s.company_code)),
       CONCAT('企业描述：该公司为答辩演示专用样本企业，数据强调岗位职责、技能要求与专业培养目标之间的可解释关联，用于展示系统在“专业对口匹配”和“跨域就业识别”两个方向的分析能力。企业项目流程覆盖需求评审、实施交付、验收复盘与持续优化，可用于演示毕业生岗位适配度评估与专业建设决策支持。')
FROM tmp_def_company_seed s
ON DUPLICATE KEY UPDATE
  main_business = VALUES(main_business),
  flagship_products = VALUES(flagship_products),
  domain_area = VALUES(domain_area),
  application_industries = VALUES(application_industries),
  province = VALUES(province),
  city = VALUES(city),
  website = VALUES(website),
  description = VALUES(description);

DROP TEMPORARY TABLE IF EXISTS tmp_def_job_seed;
CREATE TEMPORARY TABLE tmp_def_job_seed (
  job_code VARCHAR(16) PRIMARY KEY,
  company_code VARCHAR(16) NOT NULL,
  company_name VARCHAR(255) NOT NULL,
  title VARCHAR(255) NOT NULL,
  job_type ENUM('full_time', 'internship', 'contract') NOT NULL,
  min_degree ENUM('none', 'diploma', 'bachelor', 'master', 'phd') NOT NULL,
  salary_range VARCHAR(64) NOT NULL,
  target_major_code VARCHAR(32) NOT NULL,
  role_keywords TEXT NOT NULL,
  project_keywords TEXT NOT NULL,
  knowledge_keywords TEXT NOT NULL,
  tools_keywords TEXT NOT NULL
) ENGINE=InnoDB;

INSERT INTO tmp_def_job_seed VALUES
('J01','C01','星澜软件科技（答辩）','C++后端开发工程师（答辩J01）','full_time','bachelor','18k-28k','DM001','后端架构设计,服务治理,数据库优化','工业平台服务开发,高并发接口治理','C++,Linux,数据库,网络编程,软件工程','GCC,CMake,Git,MySQL,Docker'),
('J02','C01','星澜软件科技（答辩）','安全测试工程师（答辩J02）','full_time','bachelor','16k-26k','DM002','安全测试,漏洞分析,攻防演练','系统安全评估,渗透验证,应急响应','网络安全,漏洞复现,日志分析,安全策略','Python,Linux,Wireshark,扫描工具,SIEM'),
('J03','C02','华讯网络系统（答辩）','5G网络优化工程师（答辩J03）','full_time','bachelor','17k-25k','DM003','网络优化,参数调优,链路分析','基站优化专项,网络质量保障','通信原理,协议分析,指标监控,问题定位','网络优化工具,抓包工具,Linux,Python'),
('J04','C02','华讯网络系统（答辩）','射频硬件工程师（答辩J04）','full_time','bachelor','16k-24k','DM004','射频电路调试,硬件验证,性能评估','板级测试,接口联调,可靠性验证','电子电路,嵌入式基础,信号处理,测试方法','示波器,EDA工具,C,实验仪器'),
('J05','C03','智行汽车电子（答辩）','车载控制软件工程师（答辩J05）','full_time','bachelor','18k-27k','DM005','车载控制开发,CAN通信,整车联调','车机控制项目,控制策略优化','车辆工程,控制理论,CAN总线,故障诊断','C,CANoe,Matlab,测试台架,Git'),
('J06','C03','智行汽车电子（答辩）','机械结构设计工程师（答辩J06）','full_time','bachelor','15k-23k','DM006','机械设计,工艺优化,结构验证','结构件设计项目,装配工艺优化','机械设计,制造工艺,机电协同,质量控制','SolidWorks,AutoCAD,PLC,Matlab'),
('J07','C04','灵虎机器人技术（答辩）','工业控制工程师（答辩J07）','full_time','bachelor','16k-24k','DM007','PLC控制,参数整定,产线联调','自动化产线改造项目,控制优化','自动控制,传感器,工业网络,故障定位','PLC,组态软件,Matlab,工业总线工具'),
('J08','C04','灵虎机器人技术（答辩）','机器人算法工程师（答辩J08）','full_time','master','20k-32k','DM008','机器人感知,路径规划,算法调优','移动机器人项目,视觉识别优化','机器人学,机器视觉,控制算法,系统集成','ROS,Python,C++,OpenCV,Linux'),
('J09','C05','新材智造科技（答辩）','材料研发工程师（答辩J09）','full_time','master','18k-30k','DM009','材料实验设计,性能评估,工艺改进','新材料配方验证,性能对比分析','材料科学,实验分析,工艺优化,数据评估','材料测试设备,统计软件,实验系统'),
('J10','C05','新材智造科技（答辩）','化学工艺工程师（答辩J10）','full_time','bachelor','14k-22k','DM010','化学工艺优化,质量控制,安全管理','工艺放大试验,质量稳定性改进','应用化学,分析检测,工艺控制,安全规范','色谱仪,光谱仪,实验室系统,统计工具'),
('J11','C06','绿源生物医药（答辩）','生物工艺工程师（答辩J11）','full_time','bachelor','15k-23k','DM011','生物反应过程控制,参数优化','发酵工艺优化,流程稳定性提升','生物工程,实验设计,流程验证,数据分析','生物反应器,实验设备,统计软件'),
('J12','C06','绿源生物医药（答辩）','药品质量工程师（答辩J12）','full_time','bachelor','14k-22k','DM012','质量体系执行,偏差分析,合规验证','药品质量检验,验证文件闭环','制药工程,GMP,质量管理,文档规范','质量系统,实验仪器,统计软件'),
('J13','C07','建安绿色工程（答辩）','土建BIM工程师（答辩J13）','full_time','bachelor','14k-21k','DM013','BIM建模,施工协调,进度管理','工程建模项目,施工方案优化','土木工程,结构分析,施工组织,质量控制','BIM平台,AutoCAD,工程管理软件'),
('J14','C07','建安绿色工程（答辩）','环境监测工程师（答辩J14）','full_time','bachelor','13k-20k','DM014','环境监测,数据分析,治理评估','污染治理项目,监测报告编制','环境工程,监测分析,治理方案,合规评估','监测设备,GIS,数据分析软件'),
('J15','C08','云策商业管理（答辩）','市场运营专员（答辩J15）','full_time','bachelor','11k-17k','DM015','活动运营,用户增长,内容协同','品牌活动项目,转化优化分析','市场营销,消费者洞察,运营分析,复盘改进','Excel,BI,CRM,内容平台'),
('J16','C08','云策商业管理（答辩）','供应链计划工程师（答辩J16）','full_time','bachelor','12k-18k','DM016','计划编制,库存优化,流程协同','供应链协同项目,交付保障优化','物流管理,供应链分析,异常处理,计划控制','ERP,WMS,TMS,数据工具'),
('J17','C09','视界内容科技（答辩）','国际商务助理（答辩J17）','full_time','bachelor','10k-16k','DM017','跨文化沟通,商务支持,文档处理','外贸沟通项目,客户协同支持','商务英语,国际贸易,商务写作,沟通表达','办公软件,翻译工具,协作平台'),
('J18','C09','视界内容科技（答辩）','数字内容技术工程师（答辩J18）','full_time','bachelor','12k-19k','DM018','数字内容制作,交互实现,技术协作','多媒体项目,交互页面优化','数字媒体技术,内容生产,前端基础,项目协同','Premiere,AE,HTML,CSS,JS'),
('J19','C10','数衡数据科技（答辩）','统计分析师（答辩J19）','full_time','bachelor','14k-22k','DM019','统计建模,实验设计,结果解释','经营分析项目,策略评估模型','应用统计,回归分析,时间序列,抽样调查','R,Python,SQL,可视化工具'),
('J20','C10','数衡数据科技（答辩）','数据分析工程师（答辩J20）','full_time','bachelor','15k-24k','DM020','数据治理,特征工程,模型评估','数据中台项目,分析看板建设','数据科学,机器学习,数据仓库,业务建模','Python,Spark,SQL,Docker,BI');

INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, salary_range,
                 responsibilities_text, technical_projects_text, required_knowledge_skills_text,
                 required_tools_platform_text, published_at)
SELECT c.id,
       s.title,
       s.job_type,
       cs.province,
       cs.city,
       s.min_degree,
       s.salary_range,
       CONCAT('岗位职责说明：该岗位围绕', s.role_keywords, '开展工作，要求候选人能够在明确业务目标和工程约束条件下，完成需求理解、方案设计、实施交付和效果复盘的完整闭环。日常工作涉及跨团队协同、进度跟踪与风险处理，强调结果导向与质量可追踪。'),
       CONCAT('技术项目说明：入职后将参与', s.project_keywords, '等真实项目，项目周期覆盖需求评审、技术实现、联调测试、上线验证和持续迭代。岗位强调工程方法与业务价值结合，要求能够基于数据反馈持续优化方案并输出规范化文档。'),
       CONCAT('知识技能要求：应聘者需系统掌握', s.knowledge_keywords, '，并能在复杂场景中进行问题定位、方案比较和技术决策。除专业知识外，还需具备沟通表达、跨团队协同、规范执行和持续学习能力，以支撑长期岗位成长。'),
       CONCAT('工具平台要求：日常工作需熟练使用', s.tools_keywords, '等工具链完成开发、测试、部署、分析和协同。岗位重视工具使用背后的工程思维，要求能够将工具能力转化为可复用流程和稳定交付产出。除工具熟练度外，还要求候选人能够沉淀标准化操作文档并支持团队知识共享。'),
       NOW()
FROM tmp_def_job_seed s
JOIN tmp_def_company_seed cs ON cs.company_code = s.company_code
JOIN companies c ON c.name = s.company_name
WHERE NOT EXISTS (
  SELECT 1
  FROM jobs j
  WHERE j.company_id = c.id AND j.title = s.title
);

DROP TEMPORARY TABLE IF EXISTS tmp_def_major_order;
CREATE TEMPORARY TABLE tmp_def_major_order AS
SELECT s.major_code,
       s.college_code,
       s.major_name,
       s.discipline,
       s.orientation,
       s.core_courses,
       s.knowledge_skills,
       s.technical_platform,
       s.capability,
       s.industry_targets,
       c.id AS college_id,
       m.id AS major_id,
       ROW_NUMBER() OVER (ORDER BY s.major_code) AS major_rank
FROM tmp_def_major_seed s
JOIN colleges c ON c.code = s.college_code
JOIN majors m ON m.college_id = c.id AND m.name = s.major_name;

DROP TEMPORARY TABLE IF EXISTS tmp_def_seq5;
CREATE TEMPORARY TABLE tmp_def_seq5 (n INT PRIMARY KEY) ENGINE=InnoDB;
INSERT INTO tmp_def_seq5 VALUES (1), (2), (3), (4), (5);

INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, email, phone,
                     resume_text, technical_projects_text, capability_summary_text)
SELECT CONCAT('D26S', LPAD((mo.major_rank - 1) * 5 + sq.n, 4, '0')) AS student_no,
       CONCAT('答辩学生', LPAD((mo.major_rank - 1) * 5 + sq.n, 3, '0')) AS full_name,
       CASE WHEN MOD((mo.major_rank - 1) * 5 + sq.n, 2) = 0 THEN 'female' ELSE 'male' END AS gender,
       mo.college_id,
       mo.major_id,
       2026,
       CONCAT('def_student', LPAD((mo.major_rank - 1) * 5 + sq.n, 3, '0'), '@test.local') AS email,
       CONCAT('188', LPAD((mo.major_rank - 1) * 5 + sq.n, 8, '0')) AS phone,
       CONCAT('该生就读于', mo.major_name, '，学科门类为', mo.discipline, '。在校期间按照培养方案系统学习',
              mo.core_courses, '，并围绕', mo.orientation,
              '开展课程实训与综合项目实践。通过多轮任务迭代，学生逐步掌握', mo.knowledge_skills,
              '等关键能力，能够独立完成需求理解、技术实现、测试验证与结果复盘，形成较完整的工程化思维。') AS resume_text,
       CONCAT('项目经历说明：该生参与至少 2 个与', mo.industry_targets,
              '相关的综合项目，项目过程覆盖需求调研、方案设计、模块开发、联调测试与上线复盘，期间重点应用',
              mo.technical_platform,
              '等工具平台，能够在真实约束下进行问题定位与迭代优化，并输出结构化技术文档与阶段汇报。') AS technical_projects_text,
       CONCAT('能力总结：该生在学习和实践中持续强化', mo.capability,
              '等核心素养，能够在团队协作中承担明确职责并推动任务闭环。面对复杂问题时具备较好的分析能力、执行能力与沟通能力，能够将专业知识与业务目标结合，形成可落地、可评估、可持续改进的解决方案。') AS capability_summary_text
FROM tmp_def_major_order mo
CROSS JOIN tmp_def_seq5 sq
WHERE 1 = 1
ON DUPLICATE KEY UPDATE
  full_name = VALUES(full_name),
  gender = VALUES(gender),
  college_id = VALUES(college_id),
  major_id = VALUES(major_id),
  graduation_year = VALUES(graduation_year),
  email = VALUES(email),
  phone = VALUES(phone),
  resume_text = VALUES(resume_text),
  technical_projects_text = VALUES(technical_projects_text),
  capability_summary_text = VALUES(capability_summary_text);

-- 绑定 10 个学生账号用于前端学生端演示
DROP TEMPORARY TABLE IF EXISTS tmp_def_seq10;
CREATE TEMPORARY TABLE tmp_def_seq10 (n INT PRIMARY KEY) ENGINE=InnoDB;
INSERT INTO tmp_def_seq10 VALUES (1),(2),(3),(4),(5),(6),(7),(8),(9),(10);

INSERT INTO users(username, email, password_hash, role, is_active)
SELECT CONCAT('def_stu', LPAD(n, 3, '0')),
       CONCAT('def_stu', LPAD(n, 3, '0'), '@test.local'),
       'sha256:ff96673205dc722320598ebf8f88325b2ac56922d5a2164b5765868274bc0d73',
       'viewer',
       1
FROM tmp_def_seq10
ON DUPLICATE KEY UPDATE
  email = VALUES(email),
  password_hash = VALUES(password_hash),
  role = VALUES(role),
  is_active = VALUES(is_active);

UPDATE users u
JOIN students s ON s.student_no = CONCAT('D26S', LPAD(CAST(RIGHT(u.username, 3) AS UNSIGNED), 4, '0'))
SET u.linked_student_id = s.id
WHERE u.username LIKE 'def_stu%';

DROP TEMPORARY TABLE IF EXISTS tmp_def_job_ref;
CREATE TEMPORARY TABLE tmp_def_job_ref AS
SELECT s.target_major_code,
       s.title,
       j.id AS job_id,
       j.company_id
FROM tmp_def_job_seed s
JOIN companies c ON c.name = s.company_name
JOIN jobs j ON j.company_id = c.id AND j.title = s.title;

DROP TEMPORARY TABLE IF EXISTS tmp_def_major_order_cross;
CREATE TEMPORARY TABLE tmp_def_major_order_cross AS
SELECT *
FROM tmp_def_major_order;

DROP TEMPORARY TABLE IF EXISTS tmp_def_job_ref_cross;
CREATE TEMPORARY TABLE tmp_def_job_ref_cross AS
SELECT *
FROM tmp_def_job_ref;

INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status,
                               start_date, end_date, evidence_source, notes)
SELECT s.id AS student_id,
       CASE
         WHEN (CASE WHEN mo.major_rank <= 10 THEN slot_in_major <= 4 ELSE slot_in_major <= 3 END)
         THEN j_align.company_id
         ELSE j_cross.company_id
       END AS company_id,
       CASE
         WHEN (CASE WHEN mo.major_rank <= 10 THEN slot_in_major <= 4 ELSE slot_in_major <= 3 END)
         THEN j_align.job_id
         ELSE j_cross.job_id
       END AS job_id,
       CASE
         WHEN slot_in_major = 5 THEN 'offer'
         WHEN slot_in_major = 4 AND mo.major_rank > 10 THEN 'internship'
         ELSE 'signed'
       END AS employment_type,
       'active' AS employment_status,
       DATE_ADD('2026-07-01', INTERVAL student_idx DAY) AS start_date,
       NULL AS end_date,
       CASE
         WHEN slot_in_major = 5 THEN 'offer邮件（答辩样本）'
         WHEN slot_in_major = 4 AND mo.major_rank > 10 THEN '实习协议（答辩样本）'
         ELSE '三方协议（答辩样本）'
       END AS evidence_source,
       CONCAT(
         '就业情况说明：该生专业为', mo.major_name, '，当前就业岗位为',
         CASE
           WHEN (CASE WHEN mo.major_rank <= 10 THEN slot_in_major <= 4 ELSE slot_in_major <= 3 END)
           THEN j_align.title
           ELSE j_cross.title
         END,
         '。',
         CASE
           WHEN (CASE WHEN mo.major_rank <= 10 THEN slot_in_major <= 4 ELSE slot_in_major <= 3 END)
           THEN '该记录属于“专业较对口”样本，岗位关键词与专业课程、知识技能和技术平台存在较高重合度，预期匹配分数处于中高区间，可用于验证算法对正样本的识别能力。'
           ELSE '该记录属于“跨域就业”样本，岗位关键词与专业方向存在明显差异，预期匹配分数处于中低区间，可用于验证算法对非对口样本的区分能力与解释能力。'
         END,
         '该样本用于答辩阶段展示“就业记录—匹配计算—决策看板”链路的可解释性。'
       ) AS notes
FROM (
  SELECT st.id,
         st.major_id,
         CAST(RIGHT(st.student_no, 4) AS UNSIGNED) AS student_idx,
         MOD(CAST(RIGHT(st.student_no, 4) AS UNSIGNED) - 1, 5) + 1 AS slot_in_major
  FROM students st
  WHERE st.student_no LIKE 'D26S%'
) s
JOIN tmp_def_major_order mo ON mo.major_id = s.major_id
JOIN tmp_def_job_ref j_align ON j_align.target_major_code = mo.major_code
JOIN tmp_def_major_order_cross mo_cross ON mo_cross.major_rank = MOD(mo.major_rank + 6 - 1, 20) + 1
JOIN tmp_def_job_ref_cross j_cross ON j_cross.target_major_code = mo_cross.major_code
WHERE NOT EXISTS (
  SELECT 1
  FROM employment_records er
  WHERE er.student_id = s.id
);

-- 导入摘要
SELECT 'defense_showcase_seed_v1_loaded' AS message;
SELECT COUNT(*) AS defense_colleges FROM colleges WHERE code LIKE 'DC%';
SELECT COUNT(*) AS defense_majors
FROM majors m JOIN colleges c ON c.id = m.college_id
WHERE c.code LIKE 'DC%';
SELECT COUNT(*) AS defense_students FROM students WHERE student_no LIKE 'D26S%';
SELECT COUNT(*) AS defense_companies FROM companies WHERE name LIKE '%（答辩）';
SELECT COUNT(*) AS defense_jobs FROM jobs WHERE title LIKE '%（答辩J%）';
SELECT COUNT(*) AS defense_employments
FROM employment_records er JOIN students s ON s.id = er.student_id
WHERE s.student_no LIKE 'D26S%';
