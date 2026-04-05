<template>
  <section class="dbc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>

    <div class="cards">
      <article class="panel stat-card">
        <p class="stat-label">专业数量</p>
        <p class="stat-value">{{ majorCount }}</p>
        <p class="hint">已纳入看板计算</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">样本学生</p>
        <p class="stat-value">{{ totalStudents }}</p>
        <p class="hint">来自专业下学生规模</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">总体就业率</p>
        <p class="stat-value">{{ formatRate(overallEmploymentRate) }}</p>
        <p class="hint">就业人数 / 学生人数</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">总体对口率</p>
        <p class="stat-value">{{ formatRate(overallAlignmentRate) }}</p>
        <p class="hint">对口人数 / 就业人数</p>
      </article>
    </div>

    <div class="overview-grid">
      <article class="panel">
        <div class="section-head">
          <h3>任务入口</h3>
          <button class="secondary" :disabled="loading" @click="loadOverview(true)">{{ loading ? "刷新中..." : "刷新" }}</button>
        </div>
        <div class="quick-grid">
          <router-link class="quick-card" :to="{ name: 'dashboard-center-ranking' }">
            <p class="quick-title">专业排名</p>
            <p class="hint">切换指标和阈值，排序后下钻到专业详情。</p>
          </router-link>
          <router-link class="quick-card" :to="{ name: 'dashboard-center-gaps' }">
            <p class="quick-title">技能缺口</p>
            <p class="hint">按专业查看 TopN 缺口技能及出现次数。</p>
          </router-link>
          <router-link class="quick-card" :to="{ name: 'dashboard-center-insight' }">
            <p class="quick-title">AI 解读</p>
            <p class="hint">读取看板快照并调用 Agent 生成结构化结论与行动建议。</p>
          </router-link>
          <router-link class="quick-card" :to="{ name: 'dashboard-center-history' }">
            <p class="quick-title">看板记录</p>
            <p class="hint">回看最近筛选和解读参数。</p>
          </router-link>
        </div>
      </article>

      <article class="panel">
        <div class="section-head">
          <h3>排名前五</h3>
          <router-link class="quick-link" :to="{ name: 'dashboard-center-ranking' }">进入完整排名</router-link>
        </div>
        <table>
          <thead>
            <tr>
              <th>名次</th>
              <th>专业</th>
              <th>优秀毕业分</th>
              <th>对口率</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in topMajors" :key="item.major_id">
              <td>#{{ item.rank_position }}</td>
              <td>{{ item.major_name }}</td>
              <td>{{ formatScore(item.excellent_graduate_score) }}</td>
              <td>{{ formatRate(item.alignment_rate) }}</td>
            </tr>
            <tr v-if="!topMajors.length">
              <td colspan="4">暂无排名数据</td>
            </tr>
          </tbody>
        </table>
      </article>
    </div>

    <article class="panel">
      <div class="section-head">
        <h3>风险技能信号</h3>
        <router-link class="quick-link" :to="{ name: 'dashboard-center-gaps' }">进入缺口页</router-link>
      </div>
      <div class="alert-grid">
        <div v-for="item in topGapSignals" :key="item.id" class="alert-card">
          <p class="quick-title">{{ item.skill_name }}</p>
          <p class="meta">{{ item.major_name }} / {{ item.college_name }}</p>
          <p class="hint">缺口 {{ item.gap_count }} 次，涉及 {{ item.student_count }} 人，必备缺口 {{ item.mandatory_count }} 次</p>
        </div>
        <p v-if="!topGapSignals.length" class="hint">暂无缺口信号（可能样本不足或缺口为 0）。</p>
      </div>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, ref } from "vue";
import api from "../../services/api";
import { addDashboardHistory } from "../../utils/dashboardHistory";

const loading = ref(false);
const errorText = ref("");
const alignmentList = ref([]);
const gapList = ref([]);

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

const topMajors = computed(() => {
  return [...alignmentList.value]
    .sort((a, b) => Number(b.excellent_graduate_score || 0) - Number(a.excellent_graduate_score || 0))
    .slice(0, 5);
});

const topGapSignals = computed(() => {
  const rows = [];
  for (const major of gapList.value || []) {
    for (const skill of major.skills || []) {
      rows.push({
        id: `${major.major_id}-${skill.rank}-${skill.skill_name}`,
        major_name: major.major_name,
        college_name: major.college_name,
        skill_name: skill.skill_name,
        gap_count: Number(skill.gap_count || 0),
        student_count: Number(skill.student_count || 0),
        mandatory_count: Number(skill.mandatory_count || 0)
      });
    }
  }
  return rows
    .sort((a, b) => {
      const mandatoryDiff = b.mandatory_count - a.mandatory_count;
      if (mandatoryDiff !== 0) {
        return mandatoryDiff;
      }
      return b.gap_count - a.gap_count;
    })
    .slice(0, 6);
});

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "加载失败";
}

function formatRate(value) {
  return `${Number(value || 0).toFixed(2)}%`;
}

function formatScore(value) {
  return Number(value || 0).toFixed(3);
}

async function loadOverview(recordHistory = false) {
  loading.value = true;
  errorText.value = "";
  try {
    const [alignmentResp, gapResp] = await Promise.all([
      api.get("/dashboard/major-alignment", {
        params: {
          alignment_threshold: 70,
          min_students: 1,
          smoothing_k: 20
        }
      }),
      api.get("/dashboard/major-skill-gaps", {
        params: {
          top_n: 5,
          min_gap_count: 1
        }
      })
    ]);

    alignmentList.value = alignmentResp?.data?.data || [];
    gapList.value = gapResp?.data?.data || [];

    if (recordHistory) {
      addDashboardHistory({
        type: "overview",
        title: "刷新分析总览",
        summary: `覆盖 ${alignmentList.value.length} 个专业`,
        params: {
          alignment_threshold: 70,
          min_students: 1,
          smoothing_k: 20,
          top_n: 5,
          min_gap_count: 1
        }
      });
    }
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

onMounted(() => {
  loadOverview(false);
});
</script>

<style scoped>
.dbc-view {
  display: grid;
  gap: 12px;
}

.cards {
  display: grid;
  gap: 12px;
  grid-template-columns: repeat(4, minmax(0, 1fr));
}

.stat-card {
  padding: 14px;
}

.stat-label {
  margin: 0;
  color: #a6bdd8;
  font-size: 13px;
}

.stat-value {
  margin: 8px 0 4px;
  font-size: clamp(24px, 2.6vw, 34px);
  font-weight: 700;
  color: #f3f9ff;
}

.overview-grid {
  display: grid;
  gap: 12px;
  grid-template-columns: 1.08fr 0.92fr;
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

.quick-link {
  text-decoration: none;
  color: #a7d4ff;
}

.quick-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.quick-card,
.alert-card {
  text-decoration: none;
  border: 1px solid rgba(173, 211, 255, 0.24);
  border-radius: 14px;
  padding: 12px;
  background: rgba(255, 255, 255, 0.08);
  transition: all 0.2s ease;
}

.quick-card:hover {
  transform: translateY(-1px);
  background: rgba(255, 255, 255, 0.12);
}

.quick-title {
  margin: 0;
  color: #ebf4ff;
  font-size: 15px;
  font-weight: 600;
}

.alert-grid {
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

@media (max-width: 1180px) {
  .cards {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .overview-grid,
  .alert-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 640px) {
  .cards,
  .quick-grid {
    grid-template-columns: 1fr;
  }
}
</style>
