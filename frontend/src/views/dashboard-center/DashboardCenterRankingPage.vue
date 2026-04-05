<template>
  <section class="dbc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <article class="panel form-panel">
      <div class="section-head">
        <h3>排名筛选条件</h3>
      </div>
      <div class="grid">
        <label>
          算法版本（可选）
          <input v-model.trim="filters.algorithm_version" placeholder="如 v0.4-major-student" />
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
          平滑参数 k
          <input v-model.number="filters.smoothing_k" type="number" min="1" />
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
      </div>
      <div class="actions">
        <button :disabled="loading" @click="loadRanking(true)">{{ loading ? "加载中..." : "查询排名" }}</button>
        <button class="secondary" :disabled="loading" @click="resetFilters">重置</button>
      </div>
      <p class="meta-hint">
        当前配置：算法 {{ meta.algorithm_version || "latest" }}，阈值 {{ Number(meta.alignment_threshold || filters.alignment_threshold).toFixed(1) }}，最少学生 {{ meta.min_students ?? filters.min_students }}，平滑 k {{ meta.smoothing_k ?? filters.smoothing_k }}
      </p>
    </article>

    <div class="cards">
      <article class="panel stat-card">
        <p class="stat-label">专业数</p>
        <p class="stat-value">{{ majorCount }}</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">样本学生</p>
        <p class="stat-value">{{ totalStudents }}</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">总体就业率</p>
        <p class="stat-value">{{ formatRate(overallEmploymentRate) }}</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">总体对口率</p>
        <p class="stat-value">{{ formatRate(overallAlignmentRate) }}</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">加权平均分</p>
        <p class="stat-value">{{ formatScore(weightedAverageScore) }}</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">优秀毕业分</p>
        <p class="stat-value">{{ formatScore(overallExcellentScore) }}</p>
      </article>
    </div>

    <article class="panel">
      <div class="section-head">
        <h3>专业毕业表现排名</h3>
        <p class="meta">按 {{ metricLabel }} 排序</p>
      </div>
      <table>
        <thead>
          <tr>
            <th>排名</th>
            <th>专业</th>
            <th>学院</th>
            <th>学生数</th>
            <th>就业数</th>
            <th>对口数</th>
            <th>平均分</th>
            <th>优秀毕业分</th>
            <th>就业率</th>
            <th>对口率</th>
            <th>置信度</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in sortedAlignmentList" :key="item.major_id">
            <td>#{{ index + 1 }}</td>
            <td>{{ item.major_name }}</td>
            <td>{{ item.college_name }}</td>
            <td>{{ item.total_students }}</td>
            <td>{{ item.employed_students }}</td>
            <td>{{ item.aligned_students }}</td>
            <td>{{ formatScore(item.weighted_avg_match_score) }}</td>
            <td class="score-strong">{{ formatScore(item.excellent_graduate_score) }}</td>
            <td>{{ formatRate(item.employment_rate) }}</td>
            <td>{{ formatRate(item.alignment_rate) }}</td>
            <td>{{ formatConfidence(item.confidence_level) }}</td>
            <td>
              <button class="small secondary" @click="goMajorDetail(item.major_id)">下钻</button>
            </td>
          </tr>
          <tr v-if="!sortedAlignmentList.length">
            <td colspan="12">暂无排名数据</td>
          </tr>
        </tbody>
      </table>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import { useRouter } from "vue-router";
import api from "../../services/api";
import { addDashboardHistory } from "../../utils/dashboardHistory";

const router = useRouter();

const loading = ref(false);
const errorText = ref("");
const successText = ref("");
const alignmentList = ref([]);
const meta = ref({});

const filters = reactive({
  algorithm_version: "",
  alignment_threshold: 70,
  min_students: 1,
  smoothing_k: 20,
  ranking_metric: "excellent_graduate_score"
});

const metricLabelMap = {
  excellent_graduate_score: "优秀毕业分",
  alignment_rate: "对口率",
  weighted_avg_match_score: "加权平均分",
  avg_match_score: "平均分",
  employment_rate: "就业率"
};

const metricLabel = computed(() => metricLabelMap[filters.ranking_metric] || "优秀毕业分");

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
  let weighted = 0;
  let weight = 0;
  for (const item of alignmentList.value) {
    const students = Number(item.employed_with_match_students || 0);
    weighted += Number(item.weighted_avg_match_score || 0) * students;
    weight += students;
  }
  if (!weight) {
    return 0;
  }
  return weighted / weight;
});

const overallExcellentScore = computed(() => {
  let weighted = 0;
  let weight = 0;
  for (const item of alignmentList.value) {
    const students = Number(item.employed_students || 0);
    weighted += Number(item.excellent_graduate_score || 0) * students;
    weight += students;
  }
  if (!weight) {
    return 0;
  }
  return weighted / weight;
});

const sortedAlignmentList = computed(() => {
  const metric = filters.ranking_metric || "excellent_graduate_score";
  const list = [...alignmentList.value];
  list.sort((a, b) => {
    const primary = Number(b?.[metric] || 0) - Number(a?.[metric] || 0);
    if (Math.abs(primary) > 1e-9) {
      return primary;
    }
    const fallback = Number(b?.excellent_graduate_score || 0) - Number(a?.excellent_graduate_score || 0);
    if (Math.abs(fallback) > 1e-9) {
      return fallback;
    }
    return Number(a?.major_id || 0) - Number(b?.major_id || 0);
  });
  return list;
});

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "加载失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

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

function goMajorDetail(majorId) {
  const id = Number(majorId || 0);
  if (id > 0) {
    router.push(`/majors/${id}`);
  }
}

function buildParams() {
  const params = {
    alignment_threshold: Number(filters.alignment_threshold || 70),
    min_students: Number(filters.min_students || 0),
    smoothing_k: Number(filters.smoothing_k || 20)
  };
  if (filters.algorithm_version.trim()) {
    params.algorithm_version = filters.algorithm_version.trim();
  }
  return params;
}

async function loadRanking(recordHistory = false) {
  loading.value = true;
  try {
    const params = buildParams();
    const resp = await api.get("/dashboard/major-alignment", { params });
    alignmentList.value = resp?.data?.data || [];
    meta.value = resp?.data?.meta || {};
    setSuccess(`排名更新成功：共 ${alignmentList.value.length} 个专业`);

    if (recordHistory) {
      addDashboardHistory({
        type: "ranking",
        title: "查询专业排名",
        summary: `${filters.ranking_metric} / ${alignmentList.value.length} 个专业`,
        params: {
          ...params,
          ranking_metric: filters.ranking_metric
        }
      });
    }
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
  filters.smoothing_k = 20;
  filters.ranking_metric = "excellent_graduate_score";
  await loadRanking(true);
}

onMounted(() => {
  loadRanking(false);
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

.section-head h3 {
  margin: 0;
}

.grid {
  display: grid;
  grid-template-columns: repeat(5, minmax(0, 1fr));
  gap: 10px;
}

label {
  display: grid;
  gap: 6px;
}

.cards {
  display: grid;
  gap: 12px;
  grid-template-columns: repeat(6, minmax(0, 1fr));
}

.stat-card {
  padding: 12px;
}

.stat-label {
  margin: 0;
  color: #a6bdd8;
  font-size: 13px;
}

.stat-value {
  margin: 8px 0 4px;
  font-size: clamp(22px, 2.2vw, 30px);
  font-weight: 700;
  color: #f3f9ff;
}

.score-strong {
  font-weight: 700;
  color: #67d4ff;
}

@media (max-width: 1380px) {
  .grid {
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }

  .cards {
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }
}

@media (max-width: 860px) {
  .grid,
  .cards {
    grid-template-columns: 1fr;
  }
}
</style>
