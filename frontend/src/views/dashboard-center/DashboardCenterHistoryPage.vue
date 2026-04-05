<template>
  <section class="dbc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <article class="panel form-panel">
      <div class="section-head">
        <h3>看板记录筛选</h3>
      </div>
      <div class="grid">
        <label>
          记录类型
          <select v-model="filters.type">
            <option value="">全部</option>
            <option value="overview">总览</option>
            <option value="ranking">排名</option>
            <option value="gaps">缺口</option>
            <option value="insight">AI 解读</option>
          </select>
        </label>
        <label>
          数据来源
          <select v-model="filters.source">
            <option value="">全部</option>
            <option value="backend">后端</option>
            <option value="local">本地</option>
          </select>
        </label>
        <label>
          关键词
          <input v-model.trim="filters.keyword" placeholder="标题、摘要、配置" />
        </label>
      </div>
      <div class="actions">
        <button class="secondary" :disabled="loading" @click="loadHistory">{{ loading ? "刷新中..." : "刷新" }}</button>
        <button class="danger" :disabled="loading" @click="clearLocal">清空本地记录</button>
      </div>
      <p class="meta-hint">
        AI 解读记录来自后端 `dashboard_insight_runs`，其余看板操作记录保留在浏览器本地。
      </p>
    </article>

    <article class="panel">
      <div class="section-head">
        <h3>记录列表</h3>
        <p class="meta">共 {{ filteredRows.length }} 条</p>
      </div>
      <table>
        <thead>
          <tr>
            <th>时间</th>
            <th>类型</th>
            <th>标题</th>
            <th>摘要</th>
            <th>状态</th>
            <th>来源</th>
            <th>参数</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in filteredRows" :key="item.id">
            <td>{{ formatTime(item.created_at) }}</td>
            <td>{{ typeLabel(item.type) }}</td>
            <td>{{ item.title }}</td>
            <td>{{ item.summary || "-" }}</td>
            <td>{{ statusLabel(item.status) }}</td>
            <td>{{ item.source === "backend" ? "后端" : "本地" }}</td>
            <td>
              <code>{{ shortParams(item.params) }}</code>
            </td>
            <td class="row-actions">
              <button
                v-if="item.canDelete"
                class="small secondary"
                @click="removeOne(item)"
              >
                删除
              </button>
              <span v-else class="meta">-</span>
            </td>
          </tr>
          <tr v-if="filteredRows.length === 0">
            <td colspan="8">暂无记录</td>
          </tr>
        </tbody>
      </table>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import api from "../../services/api";
import {
  clearDashboardHistory,
  listDashboardHistory,
  removeDashboardHistory
} from "../../utils/dashboardHistory";

const loading = ref(false);
const errorText = ref("");
const successText = ref("");
const rows = ref([]);

const filters = reactive({
  type: "",
  source: "",
  keyword: ""
});

const filteredRows = computed(() => {
  const keyword = filters.keyword.trim().toLowerCase();
  return rows.value.filter((item) => {
    if (filters.type.length > 0 && String(item.type || "") !== filters.type) {
      return false;
    }
    if (filters.source.length > 0 && String(item.source || "") !== filters.source) {
      return false;
    }
    if (keyword.length === 0) {
      return true;
    }
    const text = `${item.title || ""} ${item.summary || ""} ${JSON.stringify(item.params || {})}`.toLowerCase();
    return text.includes(keyword);
  });
});

function typeLabel(type) {
  if (type === "ranking") {
    return "排名";
  }
  if (type === "gaps") {
    return "缺口";
  }
  if (type === "insight") {
    return "AI 解读";
  }
  return "总览";
}

function statusLabel(status) {
  if (status === "success") {
    return "成功";
  }
  if (status === "failed") {
    return "失败";
  }
  if (status === "warning") {
    return "警告";
  }
  return "-";
}

function shortParams(params) {
  const text = JSON.stringify(params || {});
  if (text.length <= 84) {
    return text;
  }
  return `${text.slice(0, 84)}...`;
}

function formatTime(value) {
  if (value) {
    const date = new Date(value);
    if (Number.isNaN(date.getTime()) === false) {
      return date.toLocaleString();
    }
  }
  return "-";
}

function mapLocalRows() {
  return listDashboardHistory().map((item) => ({
    id: `local-${item.id}`,
    localId: Number(item.id || 0),
    type: String(item.type || "overview"),
    title: String(item.title || "看板操作"),
    summary: String(item.summary || ""),
    status: String(item.status || "success"),
    params: item.params || {},
    created_at: item.created_at,
    source: "local",
    canDelete: true
  }));
}

function mapInsightRows(rowsFromApi) {
  return (rowsFromApi || []).map((item) => ({
    id: `insight-${item.id}`,
    runId: Number(item.id || 0),
    type: "insight",
    title: `AI 解读 #${item.id}`,
    summary: String(item.summary || item.error_message || ""),
    status: String(item.status || "failed"),
    params: {
      scope: item.scope,
      profile_id: item.profile_id,
      focus: item.focus,
      tone: item.tone,
      provider: item.provider,
      model: item.model,
      latency_ms: item.latency_ms
    },
    created_at: item.created_at,
    source: "backend",
    canDelete: false
  }));
}

async function loadHistory() {
  loading.value = true;
  errorText.value = "";
  successText.value = "";

  const localRows = mapLocalRows();
  let backendRows = [];

  try {
    const resp = await api.get("/dashboard/insight/runs", {
      params: {
        page: 1,
        page_size: 100,
        sort_order: "desc",
        include_payload: false
      }
    });
    backendRows = mapInsightRows(resp?.data?.data || []);
  } catch (err) {
    errorText.value = err?.response?.data?.error || err?.message || "加载后端 AI 解读记录失败";
  } finally {
    rows.value = [...backendRows, ...localRows].sort((a, b) => {
      const ta = new Date(a.created_at || 0).getTime();
      const tb = new Date(b.created_at || 0).getTime();
      return tb - ta;
    });
    loading.value = false;
  }
}

function removeOne(item) {
  if (!item?.canDelete) {
    return;
  }
  removeDashboardHistory(item.localId);
  rows.value = rows.value.filter((row) => row.id !== item.id);
  successText.value = "已删除本地记录";
}

function clearLocal() {
  const ok = window.confirm("确认清空全部本地记录吗？（不会删除后端 AI 解读记录）");
  if (ok === false) {
    return;
  }
  clearDashboardHistory();
  rows.value = rows.value.filter((item) => item.source !== "local");
  successText.value = "已清空本地记录";
}

onMounted(() => {
  loadHistory();
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
  gap: 10px;
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

label {
  display: grid;
  gap: 6px;
}

@media (max-width: 900px) {
  .grid {
    grid-template-columns: 1fr;
  }
}
</style>
