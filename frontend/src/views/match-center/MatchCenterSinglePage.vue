<template>
  <section class="mtc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="single-grid">
      <article class="panel form-panel">
        <div class="section-head">
          <h3>单条匹配参数</h3>
        </div>
        <div class="grid">
          <label>
            专业
            <select v-model.number="form.major_id">
              <option :value="0">请选择专业</option>
              <option v-for="m in majors" :key="m.id" :value="m.id">{{ m.name }}（{{ m.code }}）</option>
            </select>
          </label>
          <label>
            岗位
            <select v-model.number="form.job_id">
              <option :value="0">请选择岗位</option>
              <option v-for="j in jobs" :key="j.id" :value="j.id">{{ j.title }}（{{ j.company_name }}）</option>
            </select>
          </label>
          <label>
            算法版本
            <input v-model.trim="form.algorithm_version" placeholder="如 v0.2-major-job" />
          </label>
          <label>
            结果持久化
            <select v-model="form.persist">
              <option :value="true">是</option>
              <option :value="false">否</option>
            </select>
          </label>
        </div>
        <div class="actions">
          <button :disabled="loading" @click="runSingleMatch">{{ loading ? "计算中..." : "开始计算" }}</button>
          <button class="secondary" :disabled="loading" @click="resetForm">重置参数</button>
        </div>
      </article>

      <article class="panel">
        <div class="section-head">
          <h3>结果预览</h3>
          <span class="meta">{{ selectedMajorName }} / {{ selectedJobName }}</span>
        </div>

        <div v-if="result" class="result-wrap">
          <div class="score-grid">
            <article class="score-card">
              <p>语义分</p>
              <strong>{{ result.semantic_score }}</strong>
            </article>
            <article class="score-card">
              <p>技能分</p>
              <strong>{{ result.skill_score }}</strong>
            </article>
            <article class="score-card">
              <p>约束分</p>
              <strong>{{ result.constraint_score }}</strong>
            </article>
            <article class="score-card highlight">
              <p>最终分</p>
              <strong>{{ result.final_score }}</strong>
            </article>
          </div>

          <div class="split-grid">
            <div class="panel mini-panel">
              <h4>命中技能</h4>
              <ul class="plain-list">
                <li v-for="(item, idx) in result.matched_skills || []" :key="`matched-${idx}`">
                  {{ item.skill_name }}（权重 {{ item.job_weight }}）
                </li>
                <li v-if="!(result.matched_skills || []).length">无</li>
              </ul>
            </div>
            <div class="panel mini-panel">
              <h4>缺失技能</h4>
              <ul class="plain-list">
                <li v-for="(item, idx) in result.missing_skills || []" :key="`missing-${idx}`">
                  {{ item.skill_name }}<span v-if="item.mandatory">（必备）</span>
                </li>
                <li v-if="!(result.missing_skills || []).length">无</li>
              </ul>
            </div>
            <div class="panel mini-panel">
              <h4>约束失败项</h4>
              <ul class="plain-list">
                <li v-for="(item, idx) in result.constraint_failures || []" :key="`cons-${idx}`">
                  {{ item.type }}：{{ item.message }}
                </li>
                <li v-if="!(result.constraint_failures || []).length">无</li>
              </ul>
            </div>
          </div>
        </div>

        <div v-else class="empty-state">
          <p>尚未计算结果，填写参数后点击“开始计算”。</p>
        </div>
      </article>
    </div>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import api from "../../services/api";

const majors = ref([]);
const jobs = ref([]);

const loading = ref(false);
const errorText = ref("");
const successText = ref("");
const result = ref(null);

const form = reactive({
  major_id: 0,
  job_id: 0,
  algorithm_version: "v0.2-major-job",
  persist: true
});

const selectedMajorName = computed(() => {
  const id = Number(form.major_id || 0);
  const found = majors.value.find((m) => Number(m.id) === id);
  return found ? `${found.name}（${found.code}）` : "未选择专业";
});

const selectedJobName = computed(() => {
  const id = Number(form.job_id || 0);
  const found = jobs.value.find((j) => Number(j.id) === id);
  return found ? `${found.title}（${found.company_name}）` : "未选择岗位";
});

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "操作失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

function resetForm() {
  form.major_id = 0;
  form.job_id = 0;
  form.algorithm_version = "v0.2-major-job";
  form.persist = true;
  result.value = null;
}

async function loadOptions() {
  const [majorResp, jobResp] = await Promise.all([
    api.get("/majors", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } }),
    api.get("/jobs", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } })
  ]);
  majors.value = majorResp?.data?.data || [];
  jobs.value = jobResp?.data?.data || [];
}

async function runSingleMatch() {
  if (!form.major_id || !form.job_id) {
    setError(new Error("请先选择专业和岗位"));
    return;
  }
  loading.value = true;
  try {
    const payload = {
      major_id: Number(form.major_id),
      job_id: Number(form.job_id),
      algorithm_version: form.algorithm_version || "v0.2-major-job",
      persist: !!form.persist
    };
    const resp = await api.post("/match/major-job", payload);
    result.value = resp?.data?.data || null;
    setSuccess("单条匹配计算完成");
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

onMounted(async () => {
  try {
    await loadOptions();
  } catch (err) {
    setError(err);
  }
});
</script>

<style scoped>
.mtc-view {
  display: grid;
  gap: 12px;
}

.single-grid {
  display: grid;
  gap: 12px;
  grid-template-columns: 0.9fr 1.1fr;
}

.section-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  margin-bottom: 10px;
}

.section-head h3 {
  margin: 0;
}

.form-panel {
  display: grid;
  gap: 10px;
}

.grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

label {
  display: grid;
  gap: 6px;
}

.result-wrap {
  display: grid;
  gap: 10px;
}

.score-grid {
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(4, minmax(0, 1fr));
}

.score-card {
  border: 1px solid rgba(173, 211, 255, 0.24);
  border-radius: 14px;
  padding: 10px;
  background: rgba(255, 255, 255, 0.08);
}

.score-card p {
  margin: 0;
  color: #acc3de;
  font-size: 13px;
}

.score-card strong {
  display: block;
  margin-top: 4px;
  font-size: 24px;
  color: #f3f9ff;
}

.score-card.highlight {
  border-color: rgba(56, 189, 248, 0.5);
  background: rgba(14, 165, 233, 0.2);
}

.split-grid {
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.mini-panel {
  padding: 10px;
}

.mini-panel h4 {
  margin: 0 0 8px;
}

.empty-state {
  border: 1px dashed rgba(173, 211, 255, 0.3);
  border-radius: 14px;
  padding: 14px;
  color: #a8bfd9;
}

@media (max-width: 1200px) {
  .single-grid,
  .grid,
  .score-grid,
  .split-grid {
    grid-template-columns: 1fr;
  }
}
</style>
