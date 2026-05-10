# 机器学习轻量融合迭代 README（v1.1-hybrid-emb）

本文档对应本次已完成的系统迭代：在不新增人工大规模标注训练集的前提下，将“预训练向量语义”融合到现有匹配引擎中。

## 1. 迭代目标

- 在现有 `Jaccard + 技能加权 + 约束惩罚` 体系上，加入机器学习语义能力。
- 不引入从零训练 CNN/BERT 的高成本流程。
- 维持工程可落地与答辩可解释性：可开关、可回退、可复现。

## 2. 已执行清单（本次已完成）

- [x] 新增算法画像：`v1.1-hybrid-emb`
- [x] 匹配引擎接入预训练 embedding 语义分（OpenAI-compatible / LiteLLM）
- [x] embedding 失败自动回退到非向量语义分（不阻断匹配流程）
- [x] 语义分融合：词面语义 + 向量语义
- [x] 技能分沿用 IDF 加权（与 `v1.0-hybrid-idf` 保持一致策略）
- [x] 增加 embedding 请求超时、文本截断、向量缓存
- [x] 更新 API 合同文档
- [x] 更新环境变量示例

## 3. 核心成果与实现说明

### 3.1 新算法版本

- 新增：`v1.1-hybrid-emb`
- 适用接口：
  - `POST /api/match/single`
  - `POST /api/match/batch`
  - `POST /api/match/major-job`
  - `POST /api/match/major-job/batch`

### 3.2 评分逻辑（实现口径）

1. 词面语义分（已有）：`Jaccard + coverage`（0~100）
2. 向量语义分（新增）：专业文本与岗位文本 embedding 余弦相似度映射到 0~100
3. 语义融合（新增）：

`semantic_score = 0.45 * lexical_semantic + 0.55 * embedding_semantic`

4. 技能分：IDF 技能加权重叠（与 HybridIdf 一致）
5. 总分：

`final_score = (0.60 * semantic_score + 0.40 * skill_score) * constraint_score`

### 3.3 回退机制

以下情况会自动回退到非 embedding 语义，不影响主流程成功返回：

- embedding endpoint 未配置
- 网络错误/超时
- 上游返回异常结构

### 3.4 性能保护

- 文本长度截断：`GM_EMBEDDING_MAX_TEXT_BYTES`
- 请求超时：`GM_EMBEDDING_TIMEOUT_MS`
- 内存缓存：`GM_EMBEDDING_CACHE_SIZE`

## 4. 变更文件

- `backend/src/app_support.cc`
  - 新增 `HybridEmbedding` 算法画像
  - 新增 embedding 配置解析、HTTP 调用、向量解析、余弦计算、缓存
  - 在学生-岗位与专业-岗位两条计算链路接入 embedding 语义融合
- `backend/docs/api_contract.md`
  - 新增 `v1.1-hybrid-emb` 说明与可选值
- `backend/.env.ai.example`
  - 新增 embedding 相关环境变量示例
- `README.md`
  - 增补新算法版本说明

## 5. 部署与使用

### 5.1 环境变量（推荐）

可在 `backend/.env.ai` 中增加：

```bash
export GM_EMBEDDING_ENABLE=true
export GM_EMBEDDING_BASE_URL=http://127.0.0.1:4000
export GM_EMBEDDING_MODEL=text-embedding-3-small
# export GM_EMBEDDING_API_KEY=your_key
export GM_EMBEDDING_TIMEOUT_MS=6000
export GM_EMBEDDING_MAX_TEXT_BYTES=2400
export GM_EMBEDDING_CACHE_SIZE=2000
```

说明：`GM_EMBEDDING_API_KEY` 为空时，系统会按顺序回退读取：
`LITELLM_MASTER_KEY -> OPENAI_API_KEY`

### 5.2 调用示例

```bash
curl -X POST http://127.0.0.1:5555/api/match/single \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{
    "student_id": 1,
    "job_id": 2,
    "algorithm_version": "v1.1-hybrid-emb",
    "persist": true
  }'
```

## 6. 如何写进论文（可直接套用）

### 6.1 第 4 章（设计）建议新增小节

建议新增：`4.4.4 轻量机器学习语义融合（Hybrid-Emb）`

可写要点：

1. 研究动机：在不增加训练集标注成本下提升语义匹配能力。
2. 方法：引入预训练 embedding 作为外部语义特征，与词面语义融合。
3. 可解释性：保留技能命中/缺失与约束失败，避免黑盒不可解释。
4. 工程可用性：新增失败回退机制，保证服务稳态。

### 6.2 第 6 章（实验）建议补充

建议增加对比组：

- `v0.2-major-job`（基线）
- `v1.0-hybrid-idf`（规则增强）
- `v1.1-hybrid-emb`（轻量机器学习增强）

建议指标：

- 平均匹配分提升（平均值、标准差）
- 对口判定稳定性（阈值敏感性：60/70/80）
- 响应时延（P50/P90）
- 回退触发率（embedding 不可用场景）

### 6.3 论文中对“训练集问题”的回答模板

> 本文采用预训练语义模型提供向量表征，不进行从零监督训练，避免了大规模人工标注成本与训练资源压力。系统通过规则约束与可解释特征保持可审计性，并以工程回退机制保证生产可用性。

## 7. 答辩高频问题与参考回答

1. **为什么不用 CNN/BERT 从零训练？**  
答：本课题目标是可运行、可复现、可解释。自训练深度模型需要高质量标注集和更高算力，不符合当前毕业设计成本约束。

2. **你这个也算机器学习吗？**  
答：是。系统使用了预训练语义模型的 embedding 表征，属于机器学习模型能力的工程化应用。

3. **没有训练集，怎么证明有效？**  
答：通过对比实验（基线/IDF/Hybrid-Emb）与阈值敏感性分析，验证指标与稳定性提升。

4. **如果 embedding 服务挂了怎么办？**  
答：系统自动回退到词面语义分，不中断主流程，保证可用性。

5. **会不会变成黑盒？**  
答：不会。最终决策仍包含技能命中、缺失技能、约束失败等可解释字段。

6. **和你原有 HybridIdf 的关系？**  
答：HybridEmb 在 HybridIdf 基础上增强语义分，技能分与约束分机制保持一致。

7. **为什么要做缓存？**  
答：批量重算时文本重复度高，缓存可降低延迟与外部调用次数。

8. **向量语义是否会引入不稳定？**  
答：通过超时控制、文本截断和回退机制把风险控制在可接受范围。

9. **是否依赖特定厂商？**  
答：使用 OpenAI-compatible 接口，支持 LiteLLM 统一路由，具备供应商可切换性。

10. **后续可扩展方向？**  
答：可继续引入学习排序模型做参数学习，但保留当前可解释主链路。

## 8. 边界与后续迭代

- 当前未引入人工标注监督学习，因此“任务特定最优”仍有提升空间。
- 后续可以在不破坏当前架构的前提下，逐步引入小规模弱监督权重学习（Logistic/XGBoost）。

