<template>
  <section class="dbc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <article class="panel form-panel">
      <div class="section-head">
        <h3>AI 解读参数</h3>
      </div>
      <div class="grid">
        <label>
          Agent 配置
          <select v-model="form.profile_id">
            <option value="">请选择配置</option>
            <option v-for="item in profiles" :key="item.id" :value="item.id">
              {{ item.display_name }}（{{ item.provider }} / {{ item.model }}）
            </option>
          </select>
        </label>

        <label>
          解读范围
          <select v-model="form.scope">
            <option value="all">全量专业</option>
            <option value="college">单学院</option>
            <option value="major">单专业</option>
          </select>
        </label>

        <label>
          解读重点
          <select v-model="form.focus">
            <option value="balanced">平衡解读</option>
            <option value="alignment">对口表现优先</option>
            <option value="gap">缺口风险优先</option>
            <option value="risk">高风险专业优先</option>
            <option value="employment">就业质量优先</option>
          </select>
        </label>

        <label>
          建议语气
          <select v-model="form.tone">
            <option value="neutral">中性</option>
            <option value="strict">严格</option>
            <option value="encourage">积极</option>
            <option value="executive">管理者视角</option>
          </select>
        </label>

        <label v-if="form.scope === 'college'">
          学院
          <select v-model.number="form.college_id">
            <option :value="0">请选择学院</option>
            <option v-for="item in colleges" :key="item.id" :value="item.id">{{ item.name }}</option>
          </select>
        </label>

        <label v-if="form.scope === 'major'">
          专业
          <select v-model.number="form.major_id">
            <option :value="0">请选择专业</option>
            <option v-for="item in majors" :key="item.id" :value="item.id">{{ item.name }}（{{ item.code }}）</option>
          </select>
        </label>

        <label>
          算法版本（可选）
          <input v-model.trim="form.algorithm_version" placeholder="如 v0.4-major-student" />
        </label>
      </div>

      <label>
        自定义要求（可选）
        <textarea
          v-model.trim="form.custom_prompt"
          rows="3"
          maxlength="4000"
          placeholder="例如：请重点给出下学期可执行动作，按短期/中期分类"
        ></textarea>
      </label>

      <div class="agent-meta" v-if="selectedProfile">
        <span>当前模型：{{ selectedProfile.provider }} / {{ selectedProfile.model }}</span>
        <span>RAG：{{ selectedProfile.rag_enabled ? '已启用' : '未启用' }}</span>
        <span>入口：{{ selectedProfile.endpoint_preview || '-' }}</span>
        <span>密钥变量：{{ selectedProfile.api_key_env || '-' }}</span>
      </div>

      <div class="actions">
        <button :disabled="loading || !canRun" @click="runInsight">{{ loading ? "生成中..." : "生成解读" }}</button>
        <button class="secondary" :disabled="loading" @click="resetForm">重置参数</button>
      </div>
      <p class="meta-hint">解读结果会持久化，可在“看板记录”中复查历史 run。</p>
    </article>

    <article v-if="result" class="panel">
      <div class="section-head">
        <h3>解读结果</h3>
        <div class="result-meta">
          <span class="meta">run#{{ result.run_id || '-' }}</span>
          <span class="meta">{{ result.generated_at || '-' }}</span>
          <span class="status-pill" :class="result.status === 'success' ? 'ok' : 'fail'">
            {{ result.status === "success" ? "成功" : "失败" }}
          </span>
        </div>
      </div>

      <div class="result-grid">
        <article class="panel mini-panel">
          <h4>总体结论</h4>
          <p class="summary-text">{{ insightSummary }}</p>
          <p class="meta">
            范围：{{ result.scope_label || '-' }} / 配置：{{ result.profile_id || '-' }} / 延迟：{{ result.latency_ms || 0 }}ms
          </p>
        </article>

        <article class="panel mini-panel">
          <h4>证据摘要</h4>
          <div class="evidence-grid">
            <div class="evidence-card">
              <p class="stat-label">专业数</p>
              <p class="stat-value">{{ evidence.major_count ?? '-' }}</p>
            </div>
            <div class="evidence-card">
              <p class="stat-label">样本学生</p>
              <p class="stat-value">{{ evidence.total_students ?? '-' }}</p>
            </div>
            <div class="evidence-card">
              <p class="stat-label">总体就业率</p>
              <p class="stat-value">{{ formatRate(evidence.employment_rate) }}</p>
            </div>
            <div class="evidence-card">
              <p class="stat-label">总体对口率</p>
              <p class="stat-value">{{ formatRate(evidence.alignment_rate) }}</p>
            </div>
            <div class="evidence-card">
              <p class="stat-label">平均优秀毕业分</p>
              <p class="stat-value">{{ formatScore(evidence.avg_excellent_score) }}</p>
            </div>
            <div class="evidence-card">
              <p class="stat-label">高风险缺口技能</p>
              <p class="stat-value">{{ evidence.top_gap_skill || '-' }}</p>
            </div>
          </div>
        </article>
      </div>

      <div class="split-grid">
        <article class="panel mini-panel">
          <h4>优势点</h4>
          <ul class="plain-list">
            <li v-for="(item, index) in highlights" :key="`hl-${index}`">{{ item }}</li>
            <li v-if="highlights.length === 0">暂无</li>
          </ul>
        </article>

        <article class="panel mini-panel">
          <h4>风险点</h4>
          <ul class="plain-list">
            <li v-for="(item, index) in risks" :key="`rk-${index}`">{{ item }}</li>
            <li v-if="risks.length === 0">暂无</li>
          </ul>
        </article>

        <article class="panel mini-panel">
          <h4>行动建议</h4>
          <ul class="plain-list">
            <li v-for="(item, index) in actions" :key="`ac-${index}`">{{ item }}</li>
            <li v-if="actions.length === 0">暂无</li>
          </ul>
        </article>
      </div>

      <article v-if="retrievedContext.length > 0" class="panel mini-panel context-panel">
        <h4>RAG 召回片段</h4>
        <div class="context-grid">
          <div v-for="(ctx, index) in retrievedContext" :key="`ctx-${index}`" class="context-card">
            <p class="context-title">{{ ctx.title || `片段 ${index + 1}` }}</p>
            <p class="context-text">{{ ctx.content || '-' }}</p>
          </div>
        </div>
      </article>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import api from "../../services/api";

const loading = ref(false);
const errorText = ref("");
const successText = ref("");
const majors = ref([]);
const colleges = ref([]);
const profiles = ref([]);
const result = ref(null);

const form = reactive({
  profile_id: "",
  scope: "all",
  college_id: 0,
  major_id: 0,
  algorithm_version: "",
  focus: "balanced",
  tone: "neutral",
  custom_prompt: ""
});

const selectedProfile = computed(() =>
  profiles.value.find((item) => String(item.id || "") === String(form.profile_id || "")) || null
);

const canRun = computed(() => String(form.profile_id || "").length > 0);

const insight = computed(() => result.value?.insight || {});

const evidence = computed(() => {
  const fromInsight = insight.value?.evidence;
  if (fromInsight && typeof fromInsight === "object" && !Array.isArray(fromInsight)) {
    return fromInsight;
  }
  return result.value?.snapshot?.metrics || {};
});

const insightSummary = computed(() => {
  const text = insight.value?.summary;
  if (typeof text === "string" && text.trim().length > 0) {
    return text;
  }
  if (result.value?.status === "failed") {
    return "模型调用失败，可调整配置或密钥后重试。";
  }
  return "暂无摘要";
});

const highlights = computed(() => (Array.isArray(insight.value?.highlights) ? insight.value.highlights : []));
const risks = computed(() => (Array.isArray(insight.value?.risks) ? insight.value.risks : []));
const actions = computed(() => (Array.isArray(insight.value?.actions) ? insight.value.actions : []));
const retrievedContext = computed(() =>
  Array.isArray(result.value?.retrieved_context) ? result.value.retrieved_context : []
);

function setError(message) {
  errorText.value = String(message || "生成失败");
  successText.value = "";
}

function setSuccess(message) {
  successText.value = String(message || "操作成功");
  errorText.value = "";
}

function formatRate(value) {
  const n = Number(value || 0);
  return `${n.toFixed(2)}%`;
}

function formatScore(value) {
  const n = Number(value || 0);
  return n.toFixed(3);
}

function buildPayload() {
  const payload = {
    profile_id: form.profile_id,
    scope: form.scope,
    focus: form.focus,
    tone: form.tone,
    alignment_threshold: 70,
    min_students: 1,
    smoothing_k: 20,
    top_n: 5,
    min_gap_count: 1
  };

  if (form.scope === "college") {
    payload.college_id = Number(form.college_id || 0);
  }
  if (form.scope === "major") {
    payload.major_id = Number(form.major_id || 0);
  }

  if (form.algorithm_version.trim().length > 0) {
    payload.algorithm_version = form.algorithm_version.trim();
  }
  if (form.custom_prompt.trim().length > 0) {
    payload.custom_prompt = form.custom_prompt.trim();
  }
  return payload;
}

async function runInsight() {
  if (!form.profile_id) {
    setError("请选择 Agent 配置");
    return;
  }

  if (form.scope === "college" && Number(form.college_id || 0) <= 0) {
    setError("请选择学院");
    return;
  }
  if (form.scope === "major" && Number(form.major_id || 0) <= 0) {
    setError("请选择专业");
    return;
  }

  loading.value = true;
  try {
    const resp = await api.post("/dashboard/insight", buildPayload());
    const data = resp?.data?.data || null;
    if (!data) {
      throw new Error("响应缺少 data");
    }
    result.value = data;
    if (data.status === "success") {
      setSuccess("AI 解读已生成并写入记录");
    } else {
      setError(data.error || "AI 解读失败");
    }
  } catch (err) {
    const runData = err?.response?.data?.data;
    if (runData && typeof runData === "object") {
      result.value = runData;
    }
    const message =
      runData?.error ||
      err?.response?.data?.error ||
      err?.message ||
      "生成失败";
    setError(message);
  } finally {
    loading.value = false;
  }
}

function resetForm() {
  form.scope = "all";
  form.college_id = 0;
  form.major_id = 0;
  form.algorithm_version = "";
  form.focus = "balanced";
  form.tone = "neutral";
  form.custom_prompt = "";
  result.value = null;
  errorText.value = "";
  successText.value = "";
}

async function loadOptions() {
  const [majorResp, collegeResp, profileResp] = await Promise.all([
    api.get("/majors", { params: { page: 1, page_size: 500, sort_by: "id", sort_order: "desc" } }),
    api.get("/colleges", { params: { page: 1, page_size: 500, sort_by: "id", sort_order: "desc" } }),
    api.get("/dashboard/insight/profiles")
  ]);

  majors.value = majorResp?.data?.data || [];
  colleges.value = collegeResp?.data?.data || [];
  profiles.value = profileResp?.data?.data || [];

  const defaultProfile = String(profileResp?.data?.meta?.default_profile || "");
  if (!form.profile_id && defaultProfile.length > 0) {
    form.profile_id = defaultProfile;
  }
  if (!form.profile_id && profiles.value.length > 0) {
    form.profile_id = String(profiles.value[0].id || "");
  }
}

onMounted(async () => {
  try {
    await loadOptions();
  } catch (err) {
    const message = err?.response?.data?.error || err?.message || "加载配置失败";
    setError(message);
  }
});
</script>

<style scoped>
.dbc-view {
  display: grid;
  gap: 12px;
}

.form-panel {
  display: grid;
  gap: 10px;
}

.section-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  margin-bottom: 10px;
}

.section-head h3,
.mini-panel h4 {
  margin: 0;
}

.grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10px;
}

label {
  display: grid;
  gap: 6px;
}

textarea {
  resize: vertical;
  min-height: 84px;
}

.agent-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  font-size: 12px;
  color: #b8d1ea;
}

.result-meta {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.status-pill {
  display: inline-flex;
  align-items: center;
  border-radius: 999px;
  padding: 2px 10px;
  font-size: 12px;
  border: 1px solid transparent;
}

.status-pill.ok {
  color: #d1fae5;
  background: rgba(16, 185, 129, 0.16);
  border-color: rgba(16, 185, 129, 0.35);
}

.status-pill.fail {
  color: #fecdd3;
  background: rgba(244, 63, 94, 0.16);
  border-color: rgba(244, 63, 94, 0.35);
}

.result-grid {
  display: grid;
  gap: 10px;
  grid-template-columns: 1fr 1fr;
}

.split-grid {
  margin-top: 10px;
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.mini-panel {
  padding: 12px;
}

.summary-text {
  margin: 10px 0 0;
  line-height: 1.65;
}

.evidence-grid {
  margin-top: 10px;
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.evidence-card {
  border: 1px solid rgba(173, 211, 255, 0.24);
  border-radius: 12px;
  padding: 10px;
  background: rgba(255, 255, 255, 0.08);
}

.stat-label {
  margin: 0;
  color: #a6bdd8;
  font-size: 12px;
}

.stat-value {
  margin: 6px 0 0;
  font-size: 18px;
  font-weight: 700;
  color: #f3f9ff;
}

.context-panel {
  margin-top: 10px;
}

.context-grid {
  margin-top: 10px;
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.context-card {
  border: 1px solid rgba(173, 211, 255, 0.22);
  border-radius: 12px;
  padding: 10px;
  background: rgba(6, 20, 37, 0.28);
}

.context-title {
  margin: 0;
  font-weight: 600;
}

.context-text {
  margin: 8px 0 0;
  line-height: 1.6;
  color: #cddff3;
}

@media (max-width: 1200px) {
  .grid,
  .result-grid,
  .split-grid,
  .evidence-grid,
  .context-grid {
    grid-template-columns: 1fr;
  }
}
</style>
