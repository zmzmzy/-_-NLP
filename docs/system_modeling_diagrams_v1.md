# 系统建模图（Mermaid）

> 依据当前项目实现（`db/schema.sql` + `backend/docs/api_contract.md`）整理。

## 1) 总体架构设计图（C4Context）

```mermaid
C4Context
    title System Context diagram for Graduate Match & Decision Platform

    Enterprise_Boundary(b0, "Campus Data & Decision Boundary") {
        Person(admin, "管理员", "维护主数据、审核就业、查看看板与AI解读。")
        Person(teacher, "教师/工作人员", "维护业务数据、发起分析任务。")
        Person(viewer, "学生/查看者", "维护个人档案并提交就业信息。")

        System(websys, "Graduate Match Platform", "前后端分离平台：专业中心、岗位中心、匹配分析、审核中心、决策看板、AI解读。")

        Enterprise_Boundary(b1, "Internal Services") {
            SystemDb(db, "MySQL 8 Database", "保存主数据、审核数据、匹配结果、看板快照与AI解读历史。")
            System(aiGateway, "LiteLLM Gateway", "统一模型路由与fallback。")
            System_Ext(dify, "Dify (Optional)", "可选二层编排，用于可视化流程与策略分支。")
            System_Ext(mail, "SMTP Mail Service", "密码找回与通知邮件。")
        }
    }

    Person_Ext(ops, "运维/答辩演示人员", "启动脚本、环境配置与故障排查。")
    System_Ext(llmVendor, "LLM Provider", "OpenAI / Gemini / SiliconFlow 等模型供应方。")

    Rel(admin, websys, "使用")
    Rel(teacher, websys, "使用")
    Rel(viewer, websys, "使用")
    Rel(ops, websys, "部署与维护")

    Rel(websys, db, "读写业务数据", "SQL")
    Rel(websys, aiGateway, "调用模型网关", "HTTP")
    Rel(websys, dify, "可选编排调用", "HTTP")
    Rel(websys, mail, "发送邮件", "SMTP")

    Rel(aiGateway, llmVendor, "转发模型请求", "HTTPS")
    Rel(dify, llmVendor, "可选转发", "HTTPS")
```

## 2) 接口关系图（architecture-beta）

```mermaid
architecture-beta
    group client(internet)[Client]
    service browser(internet)[Web Browser] in client

    group fe(cloud)[Frontend]
    service vue(server)[Vue3 App (Vite)] in fe

    group be(cloud)[Backend API]
    service auth_api(server)[Auth API] in be
    service major_api(server)[College/Major/Student API] in be
    service job_api(server)[Company/Job API] in be
    service review_api(server)[Submission/Review API] in be
    service match_api(server)[Matching API] in be
    service dashboard_api(server)[Dashboard API] in be
    service insight_api(server)[Insight API] in be
    service log_api(server)[System Log API] in be

    group data(database)[Data]
    service mysql(database)[MySQL] in data

    group ai(cloud)[AI Layer]
    service litellm(server)[LiteLLM Gateway] in ai
    service dify(server)[Dify (Optional)] in ai
    service llm(cloud)[LLM Provider] in ai

    group ext(cloud)[External]
    service smtp(server)[SMTP Mail] in ext

    browser:R -- L:vue

    vue:R -- L:auth_api
    vue:R -- L:major_api
    vue:R -- L:job_api
    vue:R -- L:review_api
    vue:R -- L:match_api
    vue:R -- L:dashboard_api
    vue:R -- L:insight_api
    vue:R -- L:log_api

    auth_api:B -- T:mysql
    major_api:B -- T:mysql
    job_api:B -- T:mysql
    review_api:B -- T:mysql
    match_api:B -- T:mysql
    dashboard_api:B -- T:mysql
    insight_api:B -- T:mysql
    log_api:B -- T:mysql

    auth_api:R -- L:smtp

    insight_api:R -- L:litellm
    insight_api:R -- L:dify
    litellm:R -- L:llm
    dify:R -- L:llm
```

## 3) 数据库关系图（按族类）

### 3.1 认证与审计族

```mermaid
erDiagram
    USERS ||--o{ SYSTEM_LOGS : writes
    USERS ||--o{ PASSWORD_RESET_TOKENS : owns

    USERS {
        bigint id PK
        string username
        string email
        string role
        bool is_active
    }
    SYSTEM_LOGS {
        bigint id PK
        bigint actor_user_id FK
        string action
        string target_type
        bigint target_id
        datetime created_at
    }
    PASSWORD_RESET_TOKENS {
        bigint id PK
        bigint user_id FK
        string email
        string token
        datetime expires_at
        datetime used_at
    }
```

### 3.2 教学主数据族

```mermaid
erDiagram
    UNIVERSITIES ||--o{ COLLEGES : contains
    COLLEGES ||--o{ MAJORS : owns
    MAJORS ||--|| MAJOR_PROFILES : has_profile
    COLLEGES ||--o{ STUDENTS : has
    MAJORS ||--o{ STUDENTS : has

    UNIVERSITIES {
        bigint id PK
        string code
        string name
    }
    COLLEGES {
        bigint id PK
        bigint university_id FK
        string code
        string name
    }
    MAJORS {
        bigint id PK
        bigint college_id FK
        string code
        string name
    }
    MAJOR_PROFILES {
        bigint id PK
        bigint major_id FK
        text core_courses_text
        text knowledge_skills_text
        bigint updated_by_user_id FK
    }
    STUDENTS {
        bigint id PK
        string student_no
        string full_name
        bigint college_id FK
        bigint major_id FK
        int graduation_year
    }
```

### 3.3 岗位主数据与导入族

```mermaid
erDiagram
    COMPANIES ||--o{ JOBS : offers
    USERS ||--o{ MAJOR_IMPORT_BATCHES : triggers
    MAJOR_IMPORT_BATCHES ||--o{ MAJOR_IMPORT_ROWS : contains
    USERS ||--o{ JOB_IMPORT_BATCHES : triggers
    JOB_IMPORT_BATCHES ||--o{ JOB_IMPORT_ROWS : contains

    COMPANIES {
        bigint id PK
        string name
        text main_business
    }
    JOBS {
        bigint id PK
        bigint company_id FK
        string title
        string job_type
        string min_degree
    }
    MAJOR_IMPORT_BATCHES {
        bigint id PK
        string source_type
        string status
        bigint triggered_by_user_id FK
        datetime started_at
    }
    MAJOR_IMPORT_ROWS {
        bigint id PK
        bigint batch_id FK
        int row_no
        string row_status
        text error_message
    }
    JOB_IMPORT_BATCHES {
        bigint id PK
        string source_type
        string status
        bigint triggered_by_user_id FK
        datetime started_at
    }
    JOB_IMPORT_ROWS {
        bigint id PK
        bigint batch_id FK
        int row_no
        string row_status
        text error_message
    }
```

### 3.4 就业审核与匹配族

```mermaid
erDiagram
    STUDENTS ||--o{ EMPLOYMENT_SUBMISSIONS : submits_for
    USERS ||--o{ EMPLOYMENT_SUBMISSIONS : submits_or_reviews
    COMPANIES ||--o{ EMPLOYMENT_SUBMISSIONS : references
    JOBS ||--o{ EMPLOYMENT_SUBMISSIONS : references
    EMPLOYMENT_RECORDS ||--o{ EMPLOYMENT_SUBMISSIONS : approved_from

    STUDENTS ||--o{ EMPLOYMENT_RECORDS : has
    COMPANIES ||--o{ EMPLOYMENT_RECORDS : employs
    JOBS ||--o{ EMPLOYMENT_RECORDS : binds_job

    MAJORS ||--o{ MAJOR_SKILLS : defines
    JOBS ||--o{ JOB_SKILLS : requires
    SKILLS ||--o{ MAJOR_SKILLS : reused_by_major
    SKILLS ||--o{ JOB_SKILLS : reused_by_job

    STUDENTS ||--o{ MATCH_RESULTS : generates
    MAJORS ||--o{ MATCH_RESULTS : belongs_to
    JOBS ||--o{ MATCH_RESULTS : targets

    MAJORS ||--o{ MAJOR_JOB_MATCH_RESULTS : compares
    JOBS ||--o{ MAJOR_JOB_MATCH_RESULTS : compares
    USERS ||--o{ MAJOR_JOB_MATCH_RESULTS : created_by

    EMPLOYMENT_SUBMISSIONS {
        bigint id PK
        bigint submitter_user_id FK
        bigint student_id FK
        bigint company_id FK
        bigint job_id FK
        string status
        bigint reviewed_by_user_id FK
        bigint approved_employment_record_id FK
    }
    EMPLOYMENT_RECORDS {
        bigint id PK
        bigint student_id FK
        bigint company_id FK
        bigint job_id FK
        string employment_type
        string employment_status
    }
    SKILLS {
        bigint id PK
        string name
        string category
    }
    MAJOR_SKILLS {
        bigint id PK
        bigint major_id FK
        bigint skill_id FK
        decimal weight
    }
    JOB_SKILLS {
        bigint id PK
        bigint job_id FK
        bigint skill_id FK
        decimal weight
        bool is_mandatory
    }
    MATCH_RESULTS {
        bigint id PK
        bigint student_id FK
        bigint major_id FK
        bigint job_id FK
        decimal final_score
        datetime generated_at
    }
    MAJOR_JOB_MATCH_RESULTS {
        bigint id PK
        bigint major_id FK
        bigint job_id FK
        decimal final_score
        string run_mode
        bigint created_by_user_id FK
    }
```

### 3.5 看板与AI解读族

```mermaid
erDiagram
    MAJORS ||--o{ MAJOR_ALIGNMENT_SNAPSHOTS : snapshots
    COLLEGES ||--o{ DASHBOARD_INSIGHT_RUNS : target_college
    MAJORS ||--o{ DASHBOARD_INSIGHT_RUNS : target_major
    USERS ||--o{ DASHBOARD_INSIGHT_RUNS : created_by

    MAJOR_ALIGNMENT_SNAPSHOTS {
        bigint id PK
        bigint major_id FK
        date snapshot_date
        int total_students
        int employed_students
        decimal weighted_avg_match_score
        decimal excellent_graduate_score
    }
    DASHBOARD_INSIGHT_RUNS {
        bigint id PK
        string request_scope
        bigint target_college_id FK
        bigint target_major_id FK
        string profile_id
        string provider
        string model
        string status
        int latency_ms
        bigint created_by_user_id FK
    }
```

## 4) 全关系图（简化版，保留核心）

```mermaid
erDiagram
    UNIVERSITIES ||--o{ COLLEGES : contains
    COLLEGES ||--o{ MAJORS : owns
    MAJORS ||--|| MAJOR_PROFILES : has
    COLLEGES ||--o{ STUDENTS : has
    MAJORS ||--o{ STUDENTS : has

    COMPANIES ||--o{ JOBS : offers

    STUDENTS ||--o{ EMPLOYMENT_SUBMISSIONS : submits
    USERS ||--o{ EMPLOYMENT_SUBMISSIONS : reviews
    EMPLOYMENT_SUBMISSIONS }o--|| EMPLOYMENT_RECORDS : approved_to
    STUDENTS ||--o{ EMPLOYMENT_RECORDS : has
    COMPANIES ||--o{ EMPLOYMENT_RECORDS : employs
    JOBS ||--o{ EMPLOYMENT_RECORDS : binds

    MAJORS ||--o{ MAJOR_SKILLS : defines
    JOBS ||--o{ JOB_SKILLS : requires
    SKILLS ||--o{ MAJOR_SKILLS : mapped
    SKILLS ||--o{ JOB_SKILLS : mapped

    STUDENTS ||--o{ MATCH_RESULTS : generates
    MAJORS ||--o{ MATCH_RESULTS : referenced
    JOBS ||--o{ MATCH_RESULTS : targeted

    MAJORS ||--o{ MAJOR_JOB_MATCH_RESULTS : compares
    JOBS ||--o{ MAJOR_JOB_MATCH_RESULTS : compares

    MAJORS ||--o{ MAJOR_ALIGNMENT_SNAPSHOTS : snapshots
    COLLEGES ||--o{ DASHBOARD_INSIGHT_RUNS : insight_scope
    MAJORS ||--o{ DASHBOARD_INSIGHT_RUNS : insight_scope
```

## 5) 全关系图（详细版）

```mermaid
erDiagram
    USERS ||--o{ SYSTEM_LOGS : writes
    USERS ||--o{ PASSWORD_RESET_TOKENS : owns
    USERS ||--o{ MAJOR_IMPORT_BATCHES : triggers
    USERS ||--o{ JOB_IMPORT_BATCHES : triggers
    USERS ||--o{ EMPLOYMENT_SUBMISSIONS : submits_or_reviews
    USERS ||--o{ MAJOR_JOB_MATCH_RESULTS : creates
    USERS ||--o{ DASHBOARD_INSIGHT_RUNS : creates
    USERS ||--o{ MAJOR_PROFILES : updates

    UNIVERSITIES ||--o{ COLLEGES : contains
    COLLEGES ||--o{ MAJORS : owns
    MAJORS ||--|| MAJOR_PROFILES : has_profile
    COLLEGES ||--o{ STUDENTS : has
    MAJORS ||--o{ STUDENTS : has

    COMPANIES ||--o{ JOBS : offers
    STUDENTS ||--o{ EMPLOYMENT_RECORDS : has
    COMPANIES ||--o{ EMPLOYMENT_RECORDS : employs
    JOBS ||--o{ EMPLOYMENT_RECORDS : binds_job

    STUDENTS ||--o{ EMPLOYMENT_SUBMISSIONS : submits_for
    COMPANIES ||--o{ EMPLOYMENT_SUBMISSIONS : optional_ref_company
    JOBS ||--o{ EMPLOYMENT_SUBMISSIONS : optional_ref_job
    EMPLOYMENT_RECORDS ||--o{ EMPLOYMENT_SUBMISSIONS : approved_from

    MAJOR_IMPORT_BATCHES ||--o{ MAJOR_IMPORT_ROWS : contains
    JOB_IMPORT_BATCHES ||--o{ JOB_IMPORT_ROWS : contains

    SKILLS ||--o{ MAJOR_SKILLS : reused
    SKILLS ||--o{ JOB_SKILLS : reused
    MAJORS ||--o{ MAJOR_SKILLS : maps
    JOBS ||--o{ JOB_SKILLS : maps

    STUDENTS ||--o{ MATCH_RESULTS : generates
    MAJORS ||--o{ MATCH_RESULTS : referenced
    JOBS ||--o{ MATCH_RESULTS : targets

    MAJORS ||--o{ MAJOR_JOB_MATCH_RESULTS : compares
    JOBS ||--o{ MAJOR_JOB_MATCH_RESULTS : compares

    MAJORS ||--o{ MAJOR_ALIGNMENT_SNAPSHOTS : snapshots
    COLLEGES ||--o{ DASHBOARD_INSIGHT_RUNS : scope_college
    MAJORS ||--o{ DASHBOARD_INSIGHT_RUNS : scope_major

    USERS {
        bigint id PK
        string username
        string email
        string role
        bigint linked_student_id
        bool is_active
        datetime created_at
    }
    SYSTEM_LOGS {
        bigint id PK
        bigint actor_user_id FK
        string action
        string target_type
        bigint target_id
        json details_json
        datetime created_at
    }
    PASSWORD_RESET_TOKENS {
        bigint id PK
        bigint user_id FK
        string email
        string token
        datetime expires_at
        datetime used_at
    }
    UNIVERSITIES {
        bigint id PK
        string code
        string name
        string province
        string city
    }
    COLLEGES {
        bigint id PK
        bigint university_id FK
        string code
        string name
    }
    MAJORS {
        bigint id PK
        bigint college_id FK
        string code
        string name
    }
    MAJOR_PROFILES {
        bigint id PK
        bigint major_id FK
        text core_courses_text
        text knowledge_skills_text
        text technical_platform_text
        text capability_text
        bigint updated_by_user_id FK
    }
    COMPANIES {
        bigint id PK
        string name
        text main_business
        text flagship_products
        text domain_area
        text application_industries
    }
    JOBS {
        bigint id PK
        bigint company_id FK
        string title
        string job_type
        string min_degree
        text responsibilities_text
    }
    MAJOR_IMPORT_BATCHES {
        bigint id PK
        string source_type
        string source_filename
        string status
        bigint triggered_by_user_id FK
        int total_rows
        int valid_rows
        int invalid_rows
    }
    MAJOR_IMPORT_ROWS {
        bigint id PK
        bigint batch_id FK
        int row_no
        string row_status
        text error_message
        json raw_row_json
    }
    JOB_IMPORT_BATCHES {
        bigint id PK
        string source_type
        string source_filename
        string status
        bigint triggered_by_user_id FK
        int total_rows
        int valid_rows
        int invalid_rows
    }
    JOB_IMPORT_ROWS {
        bigint id PK
        bigint batch_id FK
        int row_no
        string row_status
        text error_message
        json raw_row_json
    }
    STUDENTS {
        bigint id PK
        string student_no
        string full_name
        string gender
        bigint college_id FK
        bigint major_id FK
        int graduation_year
    }
    EMPLOYMENT_RECORDS {
        bigint id PK
        bigint student_id FK
        bigint company_id FK
        bigint job_id FK
        string employment_type
        string employment_status
        date start_date
    }
    EMPLOYMENT_SUBMISSIONS {
        bigint id PK
        bigint submitter_user_id FK
        bigint student_id FK
        bigint company_id FK
        bigint job_id FK
        string proposed_company_name
        string proposed_job_title
        string status
        bigint reviewed_by_user_id FK
        bigint approved_employment_record_id FK
    }
    SKILLS {
        bigint id PK
        string name
        string category
    }
    MAJOR_SKILLS {
        bigint id PK
        bigint major_id FK
        bigint skill_id FK
        decimal weight
    }
    JOB_SKILLS {
        bigint id PK
        bigint job_id FK
        bigint skill_id FK
        decimal weight
        bool is_mandatory
    }
    MATCH_RESULTS {
        bigint id PK
        bigint student_id FK
        bigint major_id FK
        bigint job_id FK
        string algorithm_version
        decimal semantic_score
        decimal skill_score
        decimal constraint_score
        decimal final_score
        datetime generated_at
    }
    MAJOR_JOB_MATCH_RESULTS {
        bigint id PK
        bigint major_id FK
        bigint job_id FK
        string algorithm_version
        decimal final_score
        string run_mode
        bigint created_by_user_id FK
        datetime generated_at
    }
    MAJOR_ALIGNMENT_SNAPSHOTS {
        bigint id PK
        bigint major_id FK
        date snapshot_date
        int total_students
        int employed_students
        int aligned_students
        decimal weighted_avg_match_score
        decimal excellent_graduate_score
        int rank_position
    }
    DASHBOARD_INSIGHT_RUNS {
        bigint id PK
        string request_scope
        bigint target_college_id FK
        bigint target_major_id FK
        string profile_id
        string provider
        string model
        string focus
        string tone
        string status
        int latency_ms
        json snapshot_json
        json insight_json
        bigint created_by_user_id FK
        datetime created_at
    }
```

