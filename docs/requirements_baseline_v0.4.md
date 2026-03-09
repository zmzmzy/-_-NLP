# 需求基线（v0.4）

日期：2026-03-06

适用系统：`graduation_proj`

## 1. 课题主线（冻结）

系统主流程固定为：

1. 维护专业信息（多学院、多专业、专业画像文本）
2. 维护学生信息与就业信息（就业岗位、企业画像）
3. 计算“学生-岗位”文字关联度匹配分
4. 聚合到“专业对口率”用于专业建设决策

说明：

- `专业-岗位匹配` 保留为辅助分析能力，不作为答辩主口径。
- 岗位爬取/同步能力已下线，不作为当前版本范围。

## 2. 导师要求映射

### 2.1 就业信息（必须）

每条就业记录至少能关联以下文本信息：

- 企业：`main_business`、`flagship_products`、`domain_area`、`application_industries`
- 岗位：`title`、`technical_projects_text`、`required_knowledge_skills_text`、`required_tools_platform_text`

### 2.2 专业信息（必须）

每个专业至少包含：

- `major name`
- `core_courses_text`
- `knowledge_skills_text`
- `technical_platform_text`
- `capability_text`

样本要求：

- 至少 10-20 个不同学院的不同专业
- 不能只展示单学院样本

### 2.3 对口决策（必须）

从学生层面匹配，最终输出专业层面指标：

- 就业率
- 对口率
- 平均匹配分
- 技能缺口

## 3. 指标与口径（冻结）

### 3.1 单个学生-岗位匹配分

后端口径（已实现）：

- `semantic_score`: 文本语义重叠（0-100）
- `skill_score`: 技能重叠（0-100）
- `constraint_score`: 约束系数（0-1）
- `final_score = (0.55 * semantic_score + 0.45 * skill_score) * constraint_score`
- `final_score` 取值范围：`0-100`

### 3.2 对口判定阈值

- 默认阈值：`70`
- 判定规则：`final_score >= 70` 视为“对口”
- 阈值可在看板参数中调整（用于实验对比）

### 3.3 专业层面统计口径

- `total_students`: 该专业学生总人数
- `employed_students`: 有有效就业记录人数（非 pending）
- `aligned_students`: 有效就业中，对口人数（`final_score >= threshold`）
- `employment_rate = employed_students / total_students * 100`
- `alignment_rate = aligned_students / employed_students * 100`
- `avg_match_score`: 有效就业样本的匹配均分

## 4. 页面主流程（冻结）

1. 专业管理列表 -> 点击某专业进入专业详情
2. 专业详情维护学生与就业
3. 就业记录触发匹配（单条或批量）
4. 看板展示专业对口率与技能缺口

## 5. 非目标（v0.4 不做）

- 岗位爬虫与定时同步
- 复杂工作流审批
- 多角色细粒度权限模型扩展

## 6. 验收条件

满足以下条件即认为 v0.4 需求闭环：

1. 可展示 10-20 学院、多专业样本数据
2. 能从专业下钻到学生与就业，并得到匹配分和解释
3. 看板可输出专业就业率/对口率/均分/技能缺口
4. 结论可用于回答“该专业是否值得持续招生与培养”
