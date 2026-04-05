<template>
  <section class="mtc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>

    <div class="cards">
      <article class="panel stat-card">
        <p class="stat-label">专业样本数</p>
        <p class="stat-value">{{ stats.majors }}</p>
        <p class="hint">匹配计算的专业基数</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">岗位样本数</p>
        <p class="stat-value">{{ stats.jobs }}</p>
        <p class="hint">岗位库可用于批量分析</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">学院样本数</p>
        <p class="stat-value">{{ stats.colleges }}</p>
        <p class="hint">支持学院级重算</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">历史记录数</p>
        <p class="stat-value">{{ stats.history }}</p>
        <p class="hint">支持多条件检索</p>
      </article>
    </div>

    <div class="overview-grid">
      <article class="panel">
        <div class="section-head">
          <h3>任务流入口</h3>
          <router-link class="quick-link" :to="{ name: 'match-center-single' }">进入任务</router-link>
        </div>
        <div class="quick-grid">
          <router-link class="quick-card" :to="{ name: 'match-center-single' }">
            <p class="quick-title">单条专业 vs 岗位</p>
            <p class="hint">即时计算并展示可解释结果。</p>
          </router-link>
          <router-link class="quick-card" :to="{ name: 'match-center-multi' }">
            <p class="quick-title">单专业多岗位</p>
            <p class="hint">一个专业对多个岗位批量排序。</p>
          </router-link>
          <router-link class="quick-card" :to="{ name: 'match-center-batch' }">
            <p class="quick-title">专业/学院/全量级</p>
            <p class="hint">运行重算任务并查看执行结果。</p>
          </router-link>
          <router-link class="quick-card" :to="{ name: 'match-center-history' }">
            <p class="quick-title">历史记录查询</p>
            <p class="hint">按算法版本、运行模式过滤。</p>
          </router-link>
        </div>
      </article>

      <article class="panel">
        <div class="section-head">
          <h3>最近匹配记录</h3>
          <button class="secondary" :disabled="loading" @click="loadOverview">{{ loading ? "刷新中..." : "刷新" }}</button>
        </div>
        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>专业</th>
              <th>岗位</th>
              <th>模式</th>
              <th>最终分</th>
              <th>时间</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in recentRows" :key="item.id">
              <td>#{{ item.id }}</td>
              <td>{{ item.major_name }}</td>
              <td>{{ item.job_title }}（{{ item.company_name }}）</td>
              <td>{{ item.run_mode }}</td>
              <td>{{ item.final_score }}</td>
              <td>{{ item.generated_at }}</td>
            </tr>
            <tr v-if="!recentRows.length">
              <td colspan="6">暂无历史记录</td>
            </tr>
          </tbody>
        </table>
      </article>
    </div>

    <article class="panel">
      <div class="section-head">
        <h3>算法方式建议</h3>
      </div>
      <div class="alg-grid">
        <div class="alg-card">
          <p class="quick-title">关键词权重</p>
          <p class="hint">快速粗筛，关注课程技能标签覆盖。</p>
        </div>
        <div class="alg-card">
          <p class="quick-title">语义相似</p>
          <p class="hint">适合文本长、描述丰富的岗位场景。</p>
        </div>
        <div class="alg-card">
          <p class="quick-title">结构化维度</p>
          <p class="hint">按课程、能力、平台等维度解释评分。</p>
        </div>
        <div class="alg-card">
          <p class="quick-title">综合融合</p>
          <p class="hint">适合正式推荐和批量分析结果输出。</p>
        </div>
      </div>
    </article>
  </section>
</template>

<script setup>
import { onMounted, reactive, ref } from "vue";
import api from "../../services/api";

const loading = ref(false);
const errorText = ref("");
const recentRows = ref([]);

const stats = reactive({
  majors: 0,
  jobs: 0,
  colleges: 0,
  history: 0
});

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "加载失败";
}

function readCount(resp) {
  const body = resp?.data || {};
  if (Number.isFinite(Number(body.count))) {
    return Number(body.count);
  }
  if (Number.isFinite(Number(body?.meta?.total_count))) {
    return Number(body.meta.total_count);
  }
  if (Array.isArray(body.data)) {
    return body.data.length;
  }
  return 0;
}

async function loadOverview() {
  loading.value = true;
  errorText.value = "";
  try {
    const [majorResp, jobResp, collegeResp, historyResp] = await Promise.all([
      api.get("/majors", { params: { page: 1, page_size: 1, sort_by: "id", sort_order: "desc" } }),
      api.get("/jobs", { params: { page: 1, page_size: 1, sort_by: "id", sort_order: "desc" } }),
      api.get("/colleges", { params: { page: 1, page_size: 1, sort_by: "id", sort_order: "desc" } }),
      api.get("/match/major-job/results", { params: { page: 1, page_size: 5, sort_by: "id", sort_order: "desc" } })
    ]);

    stats.majors = readCount(majorResp);
    stats.jobs = readCount(jobResp);
    stats.colleges = readCount(collegeResp);
    stats.history = readCount(historyResp);
    recentRows.value = historyResp?.data?.data || [];
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

onMounted(loadOverview);
</script>

<style scoped>
.mtc-view {
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
  grid-template-columns: 1.1fr 0.9fr;
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
.alg-card {
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

.alg-grid {
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(4, minmax(0, 1fr));
}

@media (max-width: 1200px) {
  .cards {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .overview-grid,
  .alg-grid {
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
