<template>
  <section class="mc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>

    <div class="cards">
      <article class="panel stat-card">
        <p class="stat-label">学院总数</p>
        <p class="stat-value">{{ stats.colleges }}</p>
        <p class="hint">组织层级基础数据</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">专业总数</p>
        <p class="stat-value">{{ stats.majors }}</p>
        <p class="hint">支持画像和匹配分析</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">学生总数</p>
        <p class="stat-value">{{ stats.students }}</p>
        <p class="hint">学生画像和就业数据来源</p>
      </article>
      <article class="panel stat-card">
        <p class="stat-label">导入批次</p>
        <p class="stat-value">{{ stats.importBatches }}</p>
        <p class="hint">当前按专业导入能力统计</p>
      </article>
    </div>

    <div class="overview-grid">
      <article class="panel">
        <div class="section-head">
          <h3>快捷业务入口</h3>
          <router-link class="quick-link" :to="{ name: 'major-center-colleges' }">进入列表</router-link>
        </div>
        <div class="quick-grid">
          <router-link class="quick-card" :to="{ name: 'major-center-colleges' }">
            <p class="quick-title">学院管理</p>
            <p class="hint">学院 CRUD、院系下专业跳转</p>
          </router-link>
          <router-link class="quick-card" :to="{ name: 'major-center-majors' }">
            <p class="quick-title">专业管理</p>
            <p class="hint">专业 CRUD、专业画像维护</p>
          </router-link>
          <router-link class="quick-card" :to="{ name: 'major-center-students' }">
            <p class="quick-title">学生管理</p>
            <p class="hint">学生 CRUD、按学院专业筛选</p>
          </router-link>
          <router-link class="quick-card" :to="{ name: 'major-center-imports' }">
            <p class="quick-title">导入中心</p>
            <p class="hint">导入预检、应用和批次历史</p>
          </router-link>
        </div>
      </article>

      <article class="panel">
        <div class="section-head">
          <h3>最近导入动态</h3>
          <button class="secondary" :disabled="loading" @click="loadOverview">
            {{ loading ? "刷新中..." : "刷新" }}
          </button>
        </div>
        <table>
          <thead>
            <tr>
              <th>批次ID</th>
              <th>状态</th>
              <th>总行</th>
              <th>有效</th>
              <th>新增</th>
              <th>更新时间</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in recentBatches" :key="item.id">
              <td>#{{ item.id }}</td>
              <td>{{ item.status }}</td>
              <td>{{ item.total_rows }}</td>
              <td>{{ item.valid_rows }}</td>
              <td>{{ item.inserted_rows }}</td>
              <td>{{ item.updated_at || item.finished_at || item.started_at || '-' }}</td>
            </tr>
            <tr v-if="!recentBatches.length">
              <td colspan="6">暂无导入批次记录</td>
            </tr>
          </tbody>
        </table>
      </article>
    </div>
  </section>
</template>

<script setup>
import { onMounted, reactive, ref } from "vue";
import api from "../../services/api";

const loading = ref(false);
const errorText = ref("");
const recentBatches = ref([]);

const stats = reactive({
  colleges: 0,
  majors: 0,
  students: 0,
  importBatches: 0
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
    const [collegeResp, majorResp, studentResp, batchResp] = await Promise.all([
      api.get("/colleges", { params: { page: 1, page_size: 1, sort_by: "id", sort_order: "desc" } }),
      api.get("/majors", { params: { page: 1, page_size: 1, sort_by: "id", sort_order: "desc" } }),
      api.get("/students", { params: { page: 1, page_size: 1, sort_by: "id", sort_order: "desc" } }),
      api.get("/majors/import/batches", { params: { page: 1, page_size: 5, sort_by: "id", sort_order: "desc" } })
    ]);
    stats.colleges = readCount(collegeResp);
    stats.majors = readCount(majorResp);
    stats.students = readCount(studentResp);
    stats.importBatches = readCount(batchResp);
    recentBatches.value = batchResp?.data?.data || [];
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

onMounted(loadOverview);
</script>

<style scoped>
.mc-view {
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

.quick-card {
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

@media (max-width: 1080px) {
  .cards {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .overview-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 640px) {
  .cards {
    grid-template-columns: 1fr;
  }

  .quick-grid {
    grid-template-columns: 1fr;
  }
}
</style>
