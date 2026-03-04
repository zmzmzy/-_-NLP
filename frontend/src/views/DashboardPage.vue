<template>
  <section class="page">
    <h2>专业决策看板</h2>
    <p class="desc">
      展示各专业就业对口率、平均匹配分与核心技能缺口，辅助专业建设与招生决策。
    </p>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel form-panel">
      <h3>筛选条件</h3>
      <div class="grid">
        <label>
          毕业年份（可选）
          <input v-model.number="filters.graduation_year" type="number" min="0" placeholder="如 2026" />
        </label>
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
    </div>

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
        <h3>加权平均匹配分</h3>
        <p class="value">{{ formatScore(weightedAverageScore) }}</p>
      </article>
    </div>

    <div class="panel">
      <h3>专业对口率明细</h3>
      <table>
        <thead>
          <tr>
            <th>专业</th>
            <th>学院</th>
            <th>学生数</th>
            <th>就业数</th>
            <th>有匹配结果</th>
            <th>对口数</th>
            <th>平均分</th>
            <th>就业率</th>
            <th>对口率</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in alignmentList" :key="item.major_id">
            <td>{{ item.major_name }}</td>
            <td>{{ item.college_name }}</td>
            <td>{{ item.total_students }}</td>
            <td>{{ item.employed_students }}</td>
            <td>{{ item.employed_with_match_students }}</td>
            <td>{{ item.aligned_students }}</td>
            <td>{{ formatScore(item.avg_match_score) }}</td>
            <td>{{ formatRate(item.employment_rate) }}</td>
            <td>
              <div class="rate-cell">
                <div class="rate-bar">
                  <span :style="{ width: `${Math.max(0, Math.min(100, Number(item.alignment_rate || 0)))}%` }"></span>
                </div>
                <span>{{ formatRate(item.alignment_rate) }}</span>
              </div>
            </td>
          </tr>
          <tr v-if="!alignmentList.length">
            <td colspan="9">暂无数据</td>
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
import api from "../services/api";

const loading = ref(false);
const errorText = ref("");
const successText = ref("");
const alignmentList = ref([]);
const gapList = ref([]);

const filters = reactive({
  graduation_year: 0,
  algorithm_version: "",
  alignment_threshold: 70,
  min_students: 1,
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

const weightedAverageScore = computed(() => {
  if (!matchedStudents.value) {
    return 0;
  }
  let weighted = 0;
  for (const item of alignmentList.value) {
    weighted += Number(item.avg_match_score || 0) * Number(item.employed_with_match_students || 0);
  }
  return weighted / matchedStudents.value;
});

function formatRate(value) {
  return `${Number(value || 0).toFixed(2)}%`;
}

function formatScore(value) {
  return Number(value || 0).toFixed(3);
}

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "加载失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

function buildCommonParams() {
  const params = {};
  if (Number(filters.graduation_year || 0) > 0) {
    params.graduation_year = Number(filters.graduation_year);
  }
  if (filters.algorithm_version?.trim()) {
    params.algorithm_version = filters.algorithm_version.trim();
  }
  return params;
}

function buildAlignmentParams() {
  const params = buildCommonParams();
  params.alignment_threshold = Number(filters.alignment_threshold || 70);
  params.min_students = Number(filters.min_students || 0);
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
    gapList.value = gapResp?.data?.data || [];
    setSuccess(`看板更新成功：共 ${alignmentList.value.length} 个专业`);
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

async function resetFilters() {
  filters.graduation_year = 0;
  filters.algorithm_version = "";
  filters.alignment_threshold = 70;
  filters.min_students = 1;
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

.actions {
  margin-top: 12px;
  display: flex;
  gap: 10px;
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
