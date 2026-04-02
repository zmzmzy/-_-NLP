# 数据库表关系 Flowchart（Mermaid）

> 基于 `db/schema.sql`（graduate_match）整理。  
> 说明：`users.linked_student_id` 当前是“逻辑绑定字段”，在 schema 中未声明外键约束。

```mermaid
flowchart LR
  %% =========================
  %% Auth / Audit
  %% =========================
  subgraph AUTH["认证与审计"]
    users[(users)]
    system_logs[(system_logs)]
    password_reset_tokens[(password_reset_tokens)]
  end

  users -->|1:N actor_user_id| system_logs
  users -->|1:N user_id| password_reset_tokens

  %% =========================
  %% Education
  %% =========================
  subgraph EDU["学校-学院-专业"]
    universities[(universities)]
    colleges[(colleges)]
    majors[(majors)]
    major_profiles[(major_profiles)]
  end

  universities -->|1:N university_id| colleges
  colleges -->|1:N college_id| majors
  majors -->|1:1 major_id| major_profiles
  users -->|1:N updated_by_user_id| major_profiles

  %% =========================
  %% Job Domain
  %% =========================
  subgraph JOB["企业与岗位"]
    companies[(companies)]
    jobs[(jobs)]
  end

  companies -->|1:N company_id| jobs

  %% =========================
  %% Student / Employment
  %% =========================
  subgraph EMP["学生与就业"]
    students[(students)]
    employment_records[(employment_records)]
    employment_submissions[(employment_submissions)]
  end

  colleges -->|1:N college_id| students
  majors -->|1:N major_id| students
  users -.->|1:1 逻辑绑定 linked_student_id| students

  students -->|1:N student_id| employment_records
  companies -->|1:N company_id| employment_records
  jobs -->|1:N job_id(可空)| employment_records

  users -->|1:N submitter_user_id| employment_submissions
  users -->|1:N reviewed_by_user_id| employment_submissions
  students -->|1:N student_id| employment_submissions
  companies -->|1:N company_id(可空)| employment_submissions
  jobs -->|1:N job_id(可空)| employment_submissions
  employment_records -->|1:N approved_employment_record_id(可空)| employment_submissions

  %% =========================
  %% Skills / Matching
  %% =========================
  subgraph MATCH["技能与匹配分析"]
    skills[(skills)]
    major_skills[(major_skills)]
    job_skills[(job_skills)]
    match_results[(match_results)]
    major_job_match_results[(major_job_match_results)]
    major_alignment_snapshots[(major_alignment_snapshots)]
  end

  majors -->|1:N major_id| major_skills
  skills -->|1:N skill_id| major_skills

  jobs -->|1:N job_id| job_skills
  skills -->|1:N skill_id| job_skills

  students -->|1:N student_id| match_results
  majors -->|1:N major_id| match_results
  jobs -->|1:N job_id| match_results

  majors -->|1:N major_id| major_job_match_results
  jobs -->|1:N job_id| major_job_match_results
  users -->|1:N created_by_user_id| major_job_match_results

  majors -->|1:N major_id| major_alignment_snapshots

  %% =========================
  %% Import Batches
  %% =========================
  subgraph IMPORT["导入批次"]
    major_import_batches[(major_import_batches)]
    major_import_rows[(major_import_rows)]
    job_import_batches[(job_import_batches)]
    job_import_rows[(job_import_rows)]
  end

  users -->|1:N triggered_by_user_id| major_import_batches
  major_import_batches -->|1:N batch_id| major_import_rows

  users -->|1:N triggered_by_user_id| job_import_batches
  job_import_batches -->|1:N batch_id| job_import_rows
```

## 快速解读

- 核心主干：`universities -> colleges -> majors -> students`
- 就业主干：`companies -> jobs -> employment_records`
- 匹配核心：`match_results`（学生-专业-岗位）与 `major_alignment_snapshots`（专业聚合）
- 审核流程：`employment_submissions` 通过 `approved_employment_record_id` 回写正式就业记录
