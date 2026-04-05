<template>
  <section class="mtc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="batch-grid">
      <article class="panel form-panel">
        <div class="section-head">
          <h3>批量任务参数</h3>
        </div>
        <div class="grid">
          <label>
            作用范围
            <select v-model="form.scope">
              <option value="major">单专业</option>
              <option value="college">单学院</option>
              <option value="all">全量</option>
            </select>
          </label>
          <label v-if="form.scope === 'major'">
            专业
            <select v-model.number="form.major_id">
              <option :value="0">请选择专业</option>
              <option v-for="m in majors" :key="m.id" :value="m.id">{{ m.name }}（{{ m.code }}）</option>
            </select>
          </label>
          <label v-if="form.scope === 'college'">
            学院
            <select v-model.number="form.college_id">
              <option :value="0">请选择学院</option>
              <option v-for="c in colleges" :key="c.id" :value="c.id">{{ c.name }}</option>
            </select>
          </label>
          <label>
            算法版本
            <input v-model.trim="form.algorithm_version" placeholder="如 v0.4-major-student" />
          </label>
          <label>
            最大处理条数
            <input v-model.number="form.max_pairs" type="number" min="1" max="50000" />
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
          <button :disabled="loading" @click="runEmploymentRecompute">{{ loading ? "执行中..." : "执行批量任务" }}</button>
          <button class="secondary" :disabled="loading" @click="resetForm">重置参数</button>
        </div>
      </article>

      <article class="panel">
        <div class="section-head">
          <h3>执行摘要</h3>
        </div>
        <div class="summary-grid">
          <article class="summary-card">
            <p>范围</p>
            <strong>{{ summary.scope || '-' }}</strong>
          </article>
          <article class="summary-card">
            <p>总量</p>
            <strong>{{ summary.total_pairs || 0 }}</strong>
          </article>
          <article class="summary-card">
            <p>成功</p>
            <strong>{{ summary.success_count || 0 }}</strong>
          </article>
          <article class="summary-card">
            <p>失败</p>
            <strong>{{ summary.failed_count || 0 }}</strong>
          </article>
        </div>

        <table>
          <thead>
            <tr>
              <th>学生ID</th>
              <th>专业ID</th>
              <th>岗位ID</th>
              <th>最终分</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(item, idx) in rowsPreview" :key="`ok-${idx}`">
              <td>{{ item.student_id }}</td>
              <td>{{ item.major_id }}</td>
              <td>{{ item.job_id }}</td>
              <td>{{ item.final_score }}</td>
            </tr>
            <tr v-if="!rowsPreview.length">
              <td colspan="4">暂无成功记录</td>
            </tr>
          </tbody>
        </table>
      </article>
    </div>

    <article class="panel">
      <div class="section-head">
        <h3>失败记录</h3>
      </div>
      <table>
        <thead>
          <tr>
            <th>学生ID</th>
            <th>专业ID</th>
            <th>岗位ID</th>
            <th>状态码</th>
            <th>错误</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, idx) in failedPreview" :key="`fail-${idx}`">
            <td>{{ item.student_id }}</td>
            <td>{{ item.major_id }}</td>
            <td>{{ item.job_id }}</td>
            <td>{{ item.status }}</td>
            <td>{{ item.error }}</td>
          </tr>
          <tr v-if="!failedPreview.length">
            <td colspan="5">无失败记录</td>
          </tr>
        </tbody>
      </table>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import api from "../../services/api";

const majors = ref([]);
const colleges = ref([]);

const loading = ref(false);
const errorText = ref("");
const successText = ref("");

const rows = ref([]);
const failedRows = ref([]);
const summary = ref({});

const form = reactive({
  scope: "major",
  major_id: 0,
  college_id: 0,
  algorithm_version: "v0.4-major-student",
  persist: true,
  max_pairs: 20000
});

const rowsPreview = computed(() => (rows.value || []).slice(0, 30));
const failedPreview = computed(() => (failedRows.value || []).slice(0, 30));

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "操作失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

function resetForm() {
  form.scope = "major";
  form.major_id = 0;
  form.college_id = 0;
  form.algorithm_version = "v0.4-major-student";
  form.persist = true;
  form.max_pairs = 20000;
  rows.value = [];
  failedRows.value = [];
  summary.value = {};
}

async function loadOptions() {
  const [majorResp, collegeResp] = await Promise.all([
    api.get("/majors", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } }),
    api.get("/colleges", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } })
  ]);
  majors.value = majorResp?.data?.data || [];
  colleges.value = collegeResp?.data?.data || [];
}

async function runEmploymentRecompute() {
  const scope = String(form.scope || "major");
  let url = "";
  if (scope === "major") {
    if (!Number(form.major_id)) {
      setError(new Error("请选择专业"));
      return;
    }
    url = `/match/major/${Number(form.major_id)}/recompute`;
  } else if (scope === "college") {
    if (!Number(form.college_id)) {
      setError(new Error("请选择学院"));
      return;
    }
    url = `/match/college/${Number(form.college_id)}/recompute`;
  } else {
    url = "/match/recompute-all";
  }

  loading.value = true;
  try {
    const payload = {
      algorithm_version: form.algorithm_version || "v0.4-major-student",
      persist: !!form.persist,
      max_pairs: Math.max(1, Math.min(50000, Number(form.max_pairs || 20000)))
    };
    const resp = await api.post(url, payload);
    rows.value = resp?.data?.data || [];
    failedRows.value = resp?.data?.failed || [];
    summary.value = resp?.data?.summary || {};
    setSuccess("批量任务执行完成");
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

.batch-grid {
  display: grid;
  gap: 12px;
  grid-template-columns: 0.9fr 1.1fr;
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
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

label {
  display: grid;
  gap: 6px;
}

.summary-grid {
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  margin-bottom: 10px;
}

.summary-card {
  border: 1px solid rgba(173, 211, 255, 0.24);
  border-radius: 14px;
  padding: 10px;
  background: rgba(255, 255, 255, 0.08);
}

.summary-card p {
  margin: 0;
  color: #acc3de;
}

.summary-card strong {
  display: block;
  margin-top: 4px;
  font-size: 20px;
  color: #f3f9ff;
}

@media (max-width: 1200px) {
  .batch-grid,
  .grid,
  .summary-grid {
    grid-template-columns: 1fr;
  }
}
</style>
