<template>
  <section class="page">
    <h2>专业决策看板</h2>
    <p class="desc">
      展示各专业就业对口率、平均匹配分与核心技能缺口，辅助专业建设与招生决策。
    </p>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">筛选条件</summary>
      <div class="panel-body">
        <div class="grid">
          <label>
            算法版本（可选）
            <input v-model.trim="filters.algorithm_version" placeholder="如 v0.1-baseline" />
          </label>
          <label>
            对口阈值（0-100）
            <input v-model.number="filters.alignment_threshold" type="number" min="0" max="100" step="0.1" />
          </label>
          <label>
            最少学生数
            <input v-model.number="filters.min_students" type="number" min="0" />
          </label>
          <label>
            排序口径
            <select v-model="filters.ranking_metric">
              <option value="excellent_graduate_score">优秀毕业分</option>
              <option value="alignment_rate">对口率</option>
              <option value="weighted_avg_match_score">加权平均分</option>
              <option value="avg_match_score">平均分</option>
              <option value="employment_rate">就业率</option>
            </select>
          </label>
          <label>
            平滑参数 k
            <input v-model.number="filters.smoothing_k" type="number" min="1" />
          </label>
          <label>
            缺口 Top N（1-20）
            <input v-model.number="filters.top_n" type="number" min="1" max="20" />
          </label>
          <label>
            缺口最小出现次数
            <input v-model.number="filters.min_gap_count" type="number" min="1" />
          </label>
        </div>
        <div class="actions">
          <button :disabled="loading" @click="loadDashboard">
            {{ loading ? "加载中..." : "刷新看板" }}
          </button>
          <button class="secondary" :disabled="loading" @click="resetFilters">
            重置筛选
          </button>
        </div>
        <p class="meta-hint">
          当前对口分析配置：算法 {{ alignmentMeta.algorithm_version || "latest" }}，阈值
          {{ Number(alignmentMeta.alignment_threshold || filters.alignment_threshold).toFixed(1) }}，
          最少学生 {{ alignmentMeta.min_students ?? filters.min_students }}，平滑 k
          {{ alignmentMeta.smoothing_k ?? filters.smoothing_k }}
        </p>
      </div>
    </details>

    <div class="cards">
      <article class="card">
        <h3>专业数</h3>
        <p class="value">{{ majorCount }}</p>
      </article>
      <article class="card">
        <h3>样本学生数</h3>
        <p class="value">{{ totalStudents }}</p>
      </article>
      <article class="card">
        <h3>就业人数</h3>
        <p class="value">{{ employedStudents }}</p>
      </article>
      <article class="card">
        <h3>总体就业率</h3>
        <p class="value">{{ formatRate(overallEmploymentRate) }}</p>
      </article>
      <article class="card">
        <h3>总体对口率</h3>
        <p class="value">{{ formatRate(overallAlignmentRate) }}</p>
      </article>
      <article class="card">
        <h3>总体覆盖率</h3>
        <p class="value">{{ formatRate(overallCoverageRate) }}</p>
      </article>
      <article class="card">
        <h3>加权平均匹配分</h3>
        <p class="value">{{ formatScore(weightedAverageScore) }}</p>
      </article>
      <article class="card">
        <h3>专业原始分</h3>
        <p class="value">{{ formatScore(overallRawScore) }}</p>
      </article>
      <article class="card">
        <h3>专业优秀毕业分</h3>
        <p class="value">{{ formatScore(overallExcellentScore) }}</p>
      </article>
    </div>

    <div class="panel">
      <h3>专业毕业表现排名</h3>
      <table>
        <thead>
          <tr>
            <th>排名</th>
            <th>专业</th>
            <th>学院</th>
            <th>学生数</th>
            <th>就业数</th>
            <th>有匹配结果</th>
            <th>对口数</th>
            <th>平均分</th>
            <th>加权平均分</th>
            <th>覆盖率</th>
            <th>原始分</th>
            <th>优秀毕业分</th>
            <th>就业率</th>
            <th>对口率</th>
            <th>置信度</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, idx) in sortedAlignmentList" :key="item.major_id">
            <td>{{ idx + 1 }}</td>
            <td>{{ item.major_name }}</td>
            <td>{{ item.college_name }}</td>
            <td>{{ item.total_students }}</td>
            <td>{{ item.employed_students }}</td>
            <td>{{ item.employed_with_match_students }}</td>
            <td>{{ item.aligned_students }}</td>
            <td>{{ formatScore(item.avg_match_score) }}</td>
            <td>{{ formatScore(item.weighted_avg_match_score) }}</td>
            <td>{{ formatRate(item.coverage_rate) }}</td>
            <td>{{ formatScore(item.raw_major_score) }}</td>
            <td class="score-strong">{{ formatScore(item.excellent_graduate_score) }}</td>
            <td>{{ formatRate(item.employment_rate) }}</td>
            <td>
              <div class="rate-cell">
                <div class="rate-bar">
                  <span :style="{ width: `${Math.max(0, Math.min(100, Number(item.alignment_rate || 0)))}%` }"></span>
                </div>
                <span>{{ formatRate(item.alignment_rate) }}</span>
              </div>
            </td>
            <td>{{ formatConfidence(item.confidence_level) }}</td>
            <td>
              <button class="small secondary" @click="goMajorDetail(item.major_id)">下钻</button>
            </td>
          </tr>
          <tr v-if="!alignmentList.length">
            <td colspan="16">暂无数据</td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="panel">
      <h3>专业技能缺口（Top {{ filters.top_n }}）</h3>
      <div v-if="gapList.length" class="gap-grid">
        <article v-for="major in gapList" :key="major.major_id" class="gap-card">
          <header>
            <h4>{{ major.major_name }}</h4>
            <p>{{ major.college_name }}</p>
          </header>
          <table>
            <thead>
              <tr>
                <th>排名</th>
                <th>技能</th>
                <th>缺口次数</th>
                <th>涉及学生</th>
                <th>必备缺口</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="skill in major.skills" :key="`${major.major_id}-${skill.rank}-${skill.skill_name}`">
                <td>{{ skill.rank }}</td>
                <td>{{ skill.skill_name }}</td>
                <td>{{ skill.gap_count }}</td>
                <td>{{ skill.student_count }}</td>
                <td>{{ skill.mandatory_count }}</td>
              </tr>
              <tr v-if="!major.skills?.length">
                <td colspan="5">暂无缺口数据</td>
              </tr>
            </tbody>
          </table>
        </article>
      </div>
      <p v-else class="empty-hint">暂无技能缺口数据（可能是样本不足或缺口为 0）。</p>
    </div>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import { useRouter } from "vue-router";
import api from "../services/api";

const router = useRouter();
const loading = ref(false);
const errorText = ref("");
const successText = ref("");
const alignmentList = ref([]);
const gapList = ref([]);
const alignmentMeta = ref({});

const filters = reactive({
  algorithm_version: "",
  alignment_threshold: 70,
  min_students: 1,
  ranking_metric: "excellent_graduate_score",
  smoothing_k: 20,
  top_n: 5,
  min_gap_count: 1
});

const majorCount = computed(() => alignmentList.value.length);
const totalStudents = computed(() =>
  alignmentList.value.reduce((sum, item) => sum + Number(item.total_students || 0), 0)
);
const employedStudents = computed(() =>
  alignmentList.value.reduce((sum, item) => sum + Number(item.employed_students || 0), 0)
);
const alignedStudents = computed(() =>
  alignmentList.value.reduce((sum, item) => sum + Number(item.aligned_students || 0), 0)
);
const matchedStudents = computed(() =>
  alignmentList.value.reduce((sum, item) => sum + Number(item.employed_with_match_students || 0), 0)
);

const overallEmploymentRate = computed(() => {
  if (!totalStudents.value) {
    return 0;
  }
  return (employedStudents.value * 100) / totalStudents.value;
});

const overallAlignmentRate = computed(() => {
  if (!employedStudents.value) {
    return 0;
  }
  return (alignedStudents.value * 100) / employedStudents.value;
});

const overallCoverageRate = computed(() => {
  if (!employedStudents.value) {
    return 0;
  }
  return (matchedStudents.value * 100) / employedStudents.value;
});

const weightedAverageScore = computed(() => {
  if (!matchedStudents.value) {
    return 0;
  }
  let weighted = 0;
  for (const item of alignmentList.value) {
    weighted += Number(item.weighted_avg_match_score || 0) * Number(item.employed_with_match_students || 0);
  }
  return weighted / matchedStudents.value;
});

const overallExcellentScore = computed(() => {
  if (!employedStudents.value) {
    return 0;
  }
  let weighted = 0;
  for (const item of alignmentList.value) {
    weighted += Number(item.excellent_graduate_score || 0) * Number(item.employed_students || 0);
  }
  return weighted / employedStudents.value;
});

const overallRawScore = computed(() => {
  if (!employedStudents.value) {
    return 0;
  }
  let weighted = 0;
  for (const item of alignmentList.value) {
    weighted += Number(item.raw_major_score || 0) * Number(item.employed_students || 0);
  }
  return weighted / employedStudents.value;
});

const sortedAlignmentList = computed(() => {
  const metric = filters.ranking_metric || "excellent_graduate_score";
  const list = [...alignmentList.value];
  list.sort((a, b) => {
    const primary = Number(b?.[metric] || 0) - Number(a?.[metric] || 0);
    if (Math.abs(primary) > 1e-9) {
      return primary;
    }
    const fallbackScore = Number(b?.excellent_graduate_score || 0) - Number(a?.excellent_graduate_score || 0);
    if (Math.abs(fallbackScore) > 1e-9) {
      return fallbackScore;
    }
    const fallbackStudents = Number(b?.employed_students || 0) - Number(a?.employed_students || 0);
    if (fallbackStudents !== 0) {
      return fallbackStudents;
    }
    return Number(a?.major_id || 0) - Number(b?.major_id || 0);
  });
  return list;
});

function formatRate(value) {
  return `${Number(value || 0).toFixed(2)}%`;
}

function formatScore(value) {
  return Number(value || 0).toFixed(3);
}

function formatConfidence(value) {
  const key = String(value || "none").toLowerCase();
  if (key === "high") {
    return "高";
  }
  if (key === "medium") {
    return "中";
  }
  if (key === "low") {
    return "低";
  }
  return "无";
}

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "加载失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

function goMajorDetail(majorId) {
  const id = Number(majorId || 0);
  if (id > 0) {
    router.push(`/majors/${id}`);
  }
}

function buildCommonParams() {
  const params = {};
  if (filters.algorithm_version?.trim()) {
    params.algorithm_version = filters.algorithm_version.trim();
  }
  return params;
}

function buildAlignmentParams() {
  const params = buildCommonParams();
  params.alignment_threshold = Number(filters.alignment_threshold || 70);
  params.min_students = Number(filters.min_students || 0);
  params.smoothing_k = Number(filters.smoothing_k || 20);
  return params;
}

function buildGapParams() {
  const params = buildCommonParams();
  params.top_n = Number(filters.top_n || 5);
  params.min_gap_count = Number(filters.min_gap_count || 1);
  return params;
}

async function loadDashboard() {
  loading.value = true;
  try {
    const [alignmentResp, gapResp] = await Promise.all([
      api.get("/dashboard/major-alignment", { params: buildAlignmentParams() }),
      api.get("/dashboard/major-skill-gaps", { params: buildGapParams() })
    ]);
    alignmentList.value = alignmentResp?.data?.data || [];
    alignmentMeta.value = alignmentResp?.data?.meta || {};
    gapList.value = gapResp?.data?.data || [];
    setSuccess(`看板更新成功：共 ${alignmentList.value.length} 个专业`);
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

async function resetFilters() {
  filters.algorithm_version = "";
  filters.alignment_threshold = 70;
  filters.min_students = 1;
  filters.ranking_metric = "excellent_graduate_score";
  filters.smoothing_k = 20;
  filters.top_n = 5;
  filters.min_gap_count = 1;
  await loadDashboard();
}

onMounted(() => {
  loadDashboard();
});
</script>

<style scoped>
.page {
  background: #fff;
  padding: 20px;
  border-radius: 10px;
}

.desc {
  color: #4c5a67;
  margin-bottom: 18px;
}

.panel {
  margin-bottom: 14px;
}

.form-panel {
  border: 1px solid #dfe7ef;
  border-radius: 8px;
  background: #fafcff;
  padding: 14px;
}

.form-panel h3 {
  margin: 0 0 10px;
  font-size: 16px;
}

.collapsible-panel {
  padding: 0;
}

.panel-summary {
  list-style: none;
  cursor: pointer;
  font-weight: 600;
  padding: 14px;
  user-select: none;
}

.panel-summary::-webkit-details-marker {
  display: none;
}

.panel-summary::before {
  content: "▶";
  display: inline-block;
  margin-right: 8px;
  color: #5f86aa;
  transform: rotate(0deg);
  transition: transform 0.15s ease;
}

.collapsible-panel[open] > .panel-summary::before {
  transform: rotate(90deg);
}

.panel-body {
  padding: 0 14px 14px;
}

.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 10px;
}

label {
  display: flex;
  flex-direction: column;
  gap: 6px;
  color: #2f3b46;
  font-size: 13px;
}

input {
  border: 1px solid #c8d5e3;
  border-radius: 6px;
  padding: 8px 10px;
  background: #fff;
}

select {
  border: 1px solid #c8d5e3;
  border-radius: 6px;
  padding: 8px 10px;
  background: #fff;
}

.actions {
  margin-top: 12px;
  display: flex;
  gap: 10px;
}

.meta-hint {
  margin: 10px 0 0;
  color: #4c5a67;
  font-size: 13px;
}

button {
  border: 1px solid #1f5f99;
  background: #1f5f99;
  color: #fff;
  border-radius: 6px;
  padding: 8px 12px;
  cursor: pointer;
}

button.secondary {
  border-color: #9bb1c7;
  background: #fff;
  color: #2f3b46;
}

button:disabled {
  opacity: 0.65;
  cursor: not-allowed;
}

.cards {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  gap: 12px;
  margin-bottom: 14px;
}

.card {
  border: 1px solid #dfe7ef;
  border-radius: 8px;
  padding: 14px;
  background: #fafcff;
}

.card h3 {
  margin: 0 0 8px;
  font-size: 14px;
  color: #4c5a67;
}

.value {
  margin: 0;
  font-size: 22px;
  color: #0d2b45;
  font-weight: 600;
}

.score-strong {
  font-weight: 700;
  color: #1a4f8f;
}

.small {
  padding: 4px 8px;
  font-size: 12px;
}

table {
  width: 100%;
  border-collapse: collapse;
  margin-top: 8px;
  font-size: 13px;
}

th,
td {
  border: 1px solid #dfe7ef;
  padding: 8px 10px;
  text-align: left;
  vertical-align: middle;
}

th {
  background: #f0f5fb;
  color: #2f3b46;
}

.rate-cell {
  display: flex;
  align-items: center;
  gap: 8px;
}

.rate-bar {
  width: 110px;
  height: 8px;
  background: #e3edf8;
  border-radius: 999px;
  overflow: hidden;
}

.rate-bar span {
  display: block;
  height: 100%;
  background: linear-gradient(90deg, #1f5f99, #2f88d3);
}

.gap-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
  gap: 10px;
}

.gap-card {
  border: 1px solid #dfe7ef;
  border-radius: 8px;
  padding: 10px;
  background: #fcfdff;
}

.gap-card header {
  margin-bottom: 6px;
}

.gap-card h4 {
  margin: 0;
  font-size: 15px;
}

.gap-card p {
  margin: 4px 0 0;
  color: #4c5a67;
  font-size: 12px;
}

.empty-hint {
  margin: 8px 0 0;
  color: #6a7b8a;
}

.error {
  margin: 8px 0 12px;
  color: #c0392b;
}

.success {
  margin: 8px 0 12px;
  color: #1f7a3e;
}

@media (max-width: 800px) {
  .page {
    padding: 14px;
  }

  .rate-bar {
    width: 80px;
  }
}
</style>
