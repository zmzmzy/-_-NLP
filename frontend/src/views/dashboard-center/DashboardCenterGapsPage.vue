<template>
  <section class="dbc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <article class="panel form-panel">
      <div class="section-head">
        <h3>缺口筛选条件</h3>
      </div>
      <div class="grid">
        <label>
          算法版本（可选）
          <input v-model.trim="filters.algorithm_version" placeholder="如 v0.4-major-student" />
        </label>
        <label>
          缺口 Top N（1-20）
          <input v-model.number="filters.top_n" type="number" min="1" max="20" />
        </label>
        <label>
          缺口最小出现次数
          <input v-model.number="filters.min_gap_count" type="number" min="1" />
        </label>
        <label>
          关键词
          <input v-model.trim="filters.keyword" placeholder="专业/学院/技能" />
        </label>
      </div>
      <div class="actions">
        <button :disabled="loading" @click="loadGaps(true)">{{ loading ? "加载中..." : "查询缺口" }}</button>
        <button class="secondary" :disabled="loading" @click="resetFilters">重置</button>
      </div>
      <p class="meta-hint">
        当前配置：算法 {{ meta.algorithm_version || "latest" }}，TopN {{ meta.top_n ?? filters.top_n }}，最小出现次数 {{ meta.min_gap_count ?? filters.min_gap_count }}
      </p>
    </article>

    <div class="cards">
      <article class="panel stat-card">
        <p class="stat-label">专业数量</p>
        <p class="stat-value">{{ majorCount }}</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">缺口技能条目</p>
        <p class="stat-value">{{ totalSkillRows }}</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">必备缺口总次数</p>
        <p class="stat-value">{{ totalMandatoryCount }}</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">高风险专业</p>
        <p class="stat-value">{{ riskyMajorCount }}</p>
      </article>
    </div>

    <article class="panel">
      <div class="section-head">
        <h3>专业技能缺口</h3>
        <p class="meta">共 {{ filteredGapList.length }} 个专业</p>
      </div>

      <div v-if="filteredGapList.length > 0" class="gap-grid">
        <article v-for="major in filteredGapList" :key="major.major_id" class="gap-card">
          <header class="gap-head">
            <div>
              <h4>{{ major.major_name }}</h4>
              <p class="meta">{{ major.college_name }}</p>
            </div>
            <button class="small secondary" @click="goMajorDetail(major.major_id)">下钻</button>
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
              <tr v-if="major.skills.length === 0">
                <td colspan="5">暂无缺口技能</td>
              </tr>
            </tbody>
          </table>
        </article>
      </div>
      <p v-else class="hint">暂无缺口数据（可能样本不足或过滤条件过严）。</p>
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
const gapList = ref([]);
const meta = ref({});

const filters = reactive({
  algorithm_version: "",
  top_n: 5,
  min_gap_count: 1,
  keyword: ""
});

const filteredGapList = computed(() => {
  const keyword = filters.keyword.trim().toLowerCase();
  if (!keyword) {
    return gapList.value || [];
  }
  return (gapList.value || []).filter((major) => {
    const majorText = `${major.major_name || ""} ${major.college_name || ""}`.toLowerCase();
    if (majorText.includes(keyword)) {
      return true;
    }
    return (major.skills || []).some((skill) =>
      String(skill.skill_name || "").toLowerCase().includes(keyword)
    );
  });
});

const majorCount = computed(() => filteredGapList.value.length);
const totalSkillRows = computed(() =>
  filteredGapList.value.reduce((sum, major) => sum + (major.skills || []).length, 0)
);
const totalMandatoryCount = computed(() => {
  return filteredGapList.value.reduce((sum, major) => {
    const local = (major.skills || []).reduce(
      (skillSum, skill) => skillSum + Number(skill.mandatory_count || 0),
      0
    );
    return sum + local;
  }, 0);
});

const riskyMajorCount = computed(() => {
  return filteredGapList.value.filter((major) => {
    const topSkill = (major.skills || [])[0];
    if (!topSkill) {
      return false;
    }
    return Number(topSkill.mandatory_count || 0) >= 2 || Number(topSkill.gap_count || 0) >= 4;
  }).length;
});

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

function buildParams() {
  const params = {
    top_n: Math.max(1, Math.min(20, Number(filters.top_n || 5))),
    min_gap_count: Math.max(1, Number(filters.min_gap_count || 1))
  };
  if (filters.algorithm_version.trim()) {
    params.algorithm_version = filters.algorithm_version.trim();
  }
  return params;
}

async function loadGaps(recordHistory = false) {
  loading.value = true;
  try {
    const params = buildParams();
    const resp = await api.get("/dashboard/major-skill-gaps", { params });
    gapList.value = resp?.data?.data || [];
    meta.value = resp?.data?.meta || {};
    setSuccess(`缺口更新成功：共 ${gapList.value.length} 个专业`);

    if (recordHistory) {
      addDashboardHistory({
        type: "gaps",
        title: "查询技能缺口",
        summary: `TopN ${params.top_n} / ${gapList.value.length} 个专业`,
        params
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
  filters.top_n = 5;
  filters.min_gap_count = 1;
  filters.keyword = "";
  await loadGaps(true);
}

onMounted(() => {
  loadGaps(false);
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
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 10px;
}

label {
  display: grid;
  gap: 6px;
}

.cards {
  display: grid;
  gap: 12px;
  grid-template-columns: repeat(4, minmax(0, 1fr));
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

.gap-grid {
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.gap-card {
  border: 1px solid rgba(173, 211, 255, 0.24);
  border-radius: 14px;
  padding: 12px;
  background: rgba(255, 255, 255, 0.08);
}

.gap-head {
  margin-bottom: 8px;
  display: flex;
  justify-content: space-between;
  gap: 10px;
}

.gap-head h4 {
  margin: 0;
}

@media (max-width: 1200px) {
  .grid,
  .cards,
  .gap-grid {
    grid-template-columns: 1fr;
  }
}
</style>
