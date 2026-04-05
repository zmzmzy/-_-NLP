<template>
  <section class="mtc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <article class="panel form-panel">
      <div class="section-head">
        <h3>历史检索条件</h3>
      </div>
      <div class="grid">
        <label>
          专业
          <select v-model.number="filters.major_id">
            <option :value="0">全部专业</option>
            <option v-for="m in majors" :key="m.id" :value="m.id">{{ m.name }}（{{ m.code }}）</option>
          </select>
        </label>
        <label>
          岗位
          <select v-model.number="filters.job_id">
            <option :value="0">全部岗位</option>
            <option v-for="j in jobs" :key="j.id" :value="j.id">{{ j.title }}（{{ j.company_name }}）</option>
          </select>
        </label>
        <label>
          算法版本
          <input v-model.trim="filters.algorithm_version" placeholder="留空表示全部" />
        </label>
        <label>
          运行模式
          <select v-model="filters.run_mode">
            <option value="">全部</option>
            <option value="single">single</option>
            <option value="batch">batch</option>
          </select>
        </label>
      </div>
      <div class="actions">
        <button :disabled="loading" @click="searchHistory">{{ loading ? "查询中..." : "查询" }}</button>
        <button class="secondary" :disabled="loading" @click="resetFilters">重置</button>
      </div>
    </article>

    <article class="panel">
      <div class="section-head">
        <h3>历史结果</h3>
        <p class="meta">第 {{ page }} / {{ totalPages }} 页，共 {{ totalCount }} 条</p>
      </div>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>专业</th>
            <th>岗位</th>
            <th>模式</th>
            <th>算法版本</th>
            <th>最终分</th>
            <th>时间</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in rows" :key="item.id">
            <td>{{ rowNo(index) }}</td>
            <td>{{ item.major_name }}</td>
            <td>{{ item.job_title }}（{{ item.company_name }}）</td>
            <td>{{ item.run_mode }}</td>
            <td>{{ item.algorithm_version }}</td>
            <td>{{ item.final_score }}</td>
            <td>{{ item.generated_at }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="removeHistoryItem(item)">删除</button>
            </td>
          </tr>
          <tr v-if="!rows.length">
            <td colspan="8">暂无匹配历史</td>
          </tr>
        </tbody>
      </table>
      <div class="actions page-actions">
        <button class="secondary" :disabled="loading || page <= 1" @click="prevPage">上一页</button>
        <button class="secondary" :disabled="loading || page >= totalPages" @click="nextPage">下一页</button>
      </div>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import api from "../../services/api";

const majors = ref([]);
const jobs = ref([]);
const rows = ref([]);

const loading = ref(false);
const errorText = ref("");
const successText = ref("");

const page = ref(1);
const pageSize = ref(20);
const totalCount = ref(0);

const filters = reactive({
  major_id: 0,
  job_id: 0,
  algorithm_version: "",
  run_mode: ""
});

const totalPages = computed(() => {
  const total = Number(totalCount.value || 0);
  const size = Number(pageSize.value || 20);
  if (total <= 0) {
    return 1;
  }
  return Math.max(1, Math.ceil(total / size));
});

function rowNo(index) {
  const p = Math.max(1, Number(page.value || 1));
  const s = Math.max(1, Number(pageSize.value || 20));
  return (p - 1) * s + Number(index) + 1;
}

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "操作失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

async function loadOptions() {
  const [majorResp, jobResp] = await Promise.all([
    api.get("/majors", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } }),
    api.get("/jobs", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } })
  ]);
  majors.value = majorResp?.data?.data || [];
  jobs.value = jobResp?.data?.data || [];
}

async function loadHistory() {
  loading.value = true;
  try {
    const params = {
      page: Number(page.value || 1),
      page_size: Number(pageSize.value || 20),
      sort_by: "id",
      sort_order: "desc"
    };
    if (Number(filters.major_id || 0) > 0) {
      params.major_id = Number(filters.major_id);
    }
    if (Number(filters.job_id || 0) > 0) {
      params.job_id = Number(filters.job_id);
    }
    if (filters.algorithm_version.trim()) {
      params.algorithm_version = filters.algorithm_version.trim();
    }
    if (filters.run_mode.trim()) {
      params.run_mode = filters.run_mode.trim();
    }

    const resp = await api.get("/match/major-job/results", { params });
    rows.value = resp?.data?.data || [];
    totalCount.value = Number(resp?.data?.count || 0);
    const maxPage = Math.max(1, Math.ceil(totalCount.value / Number(pageSize.value || 20)));
    if (page.value > maxPage) {
      page.value = maxPage;
    }
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

async function searchHistory() {
  page.value = 1;
  await loadHistory();
}

async function resetFilters() {
  filters.major_id = 0;
  filters.job_id = 0;
  filters.algorithm_version = "";
  filters.run_mode = "";
  page.value = 1;
  await loadHistory();
}

async function prevPage() {
  if (page.value <= 1) {
    return;
  }
  page.value -= 1;
  await loadHistory();
}

async function nextPage() {
  if (page.value >= totalPages.value) {
    return;
  }
  page.value += 1;
  await loadHistory();
}

async function removeHistoryItem(item) {
  if (!window.confirm(`确认删除匹配记录 #${item.id} 吗？`)) {
    return;
  }
  try {
    await api.delete(`/match/major-job/results/${item.id}`);
    setSuccess(`已删除匹配记录 #${item.id}`);
    await loadHistory();
  } catch (err) {
    setError(err);
  }
}

onMounted(async () => {
  try {
    await loadOptions();
    await loadHistory();
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

.page-actions {
  margin-top: 10px;
}

@media (max-width: 1200px) {
  .grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 760px) {
  .grid {
    grid-template-columns: 1fr;
  }
}
</style>
