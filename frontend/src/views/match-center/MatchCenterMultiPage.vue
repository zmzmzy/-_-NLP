<template>
  <section class="mtc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="multi-grid">
      <article class="panel form-panel">
        <div class="section-head">
          <h3>多岗位匹配参数</h3>
        </div>
        <div class="grid">
          <label>
            专业
            <select v-model.number="form.major_id">
              <option :value="0">请选择专业</option>
              <option v-for="m in majors" :key="m.id" :value="m.id">{{ m.name }}（{{ m.code }}）</option>
            </select>
          </label>
          <label>
            岗位ID列表（可选）
            <input v-model.trim="form.job_ids_text" placeholder="示例：1001,1002；留空表示全岗位" />
          </label>
          <label>
            算法版本
            <input v-model.trim="form.algorithm_version" placeholder="如 v0.2-major-job" />
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
          <button :disabled="loading" @click="runBatchMatch">{{ loading ? "计算中..." : "执行任务" }}</button>
          <button class="secondary" :disabled="loading" @click="resetForm">重置参数</button>
        </div>
        <p class="meta">当前已加载岗位样本：{{ jobs.length }} 条（留空将全量参与匹配）</p>
      </article>

      <article class="panel">
        <div class="section-head">
          <h3>执行摘要</h3>
        </div>
        <div class="summary-grid">
          <article class="summary-card">
            <p>总组合</p>
            <strong>{{ summary.total_pairs }}</strong>
          </article>
          <article class="summary-card">
            <p>成功</p>
            <strong>{{ summary.success_count }}</strong>
          </article>
          <article class="summary-card">
            <p>失败</p>
            <strong>{{ summary.failed_count }}</strong>
          </article>
        </div>

        <table>
          <thead>
            <tr>
              <th>#</th>
              <th>专业</th>
              <th>岗位</th>
              <th>最终分</th>
              <th>语义分</th>
              <th>技能分</th>
              <th>约束分</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(item, idx) in sortedRows" :key="`ok-${idx}`">
              <td>{{ idx + 1 }}</td>
              <td>{{ majorNameById[item.major_id] || item.major_id }}</td>
              <td>{{ jobTitleById[item.job_id] || item.job_id }}</td>
              <td>{{ item.final_score }}</td>
              <td>{{ item.semantic_score }}</td>
              <td>{{ item.skill_score }}</td>
              <td>{{ item.constraint_score }}</td>
            </tr>
            <tr v-if="!sortedRows.length">
              <td colspan="7">暂无成功结果</td>
            </tr>
          </tbody>
        </table>
      </article>
    </div>

    <article class="panel">
      <div class="section-head">
        <h3>失败结果</h3>
      </div>
      <table>
        <thead>
          <tr>
            <th>专业ID</th>
            <th>岗位ID</th>
            <th>状态码</th>
            <th>错误信息</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, idx) in failedRows" :key="`fail-${idx}`">
            <td>{{ item.major_id }}</td>
            <td>{{ item.job_id }}</td>
            <td>{{ item.status }}</td>
            <td>{{ item.error }}</td>
          </tr>
          <tr v-if="!failedRows.length">
            <td colspan="4">无失败记录</td>
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
const jobs = ref([]);

const loading = ref(false);
const errorText = ref("");
const successText = ref("");

const rows = ref([]);
const failedRows = ref([]);
const summary = reactive({
  total_pairs: 0,
  success_count: 0,
  failed_count: 0
});

const form = reactive({
  major_id: 0,
  job_ids_text: "",
  algorithm_version: "v0.2-major-job",
  persist: true
});

const majorNameById = computed(() => {
  const map = {};
  for (const item of majors.value) {
    map[item.id] = item.name;
  }
  return map;
});

const jobTitleById = computed(() => {
  const map = {};
  for (const item of jobs.value) {
    map[item.id] = `${item.title}${item.company_name ? `（${item.company_name}）` : ""}`;
  }
  return map;
});

const sortedRows = computed(() => {
  return [...rows.value].sort((a, b) => Number(b.final_score || 0) - Number(a.final_score || 0));
});

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "操作失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

function parseIdList(text) {
  return [...new Set(
    String(text || "")
      .split(/[,\s;，；]+/)
      .map((x) => Number(x.trim()))
      .filter((x) => Number.isInteger(x) && x > 0)
  )];
}

function resetForm() {
  form.major_id = 0;
  form.job_ids_text = "";
  form.algorithm_version = "v0.2-major-job";
  form.persist = true;
  rows.value = [];
  failedRows.value = [];
  summary.total_pairs = 0;
  summary.success_count = 0;
  summary.failed_count = 0;
}

async function loadOptions() {
  const [majorResp, jobResp] = await Promise.all([
    api.get("/majors", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } }),
    api.get("/jobs", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } })
  ]);
  majors.value = majorResp?.data?.data || [];
  jobs.value = jobResp?.data?.data || [];
}

async function runBatchMatch() {
  if (!form.major_id) {
    setError(new Error("请先选择专业"));
    return;
  }

  let jobIds = parseIdList(form.job_ids_text);
  if (!jobIds.length) {
    jobIds = jobs.value.map((item) => Number(item.id)).filter((id) => Number.isInteger(id) && id > 0);
  }
  if (!jobIds.length) {
    setError(new Error("未找到可用于匹配的岗位ID"));
    return;
  }

  loading.value = true;
  try {
    const payload = {
      major_ids: [Number(form.major_id)],
      job_ids: jobIds,
      algorithm_version: form.algorithm_version || "v0.2-major-job",
      persist: !!form.persist
    };
    const resp = await api.post("/match/major-job/batch", payload);
    rows.value = resp?.data?.data || [];
    failedRows.value = resp?.data?.failed || [];

    const sum = resp?.data?.summary || {};
    summary.total_pairs = Number(sum.total_pairs || 0);
    summary.success_count = Number(sum.success_count || 0);
    summary.failed_count = Number(sum.failed_count || 0);
    setSuccess("批量匹配计算完成");
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

.multi-grid {
  display: grid;
  gap: 12px;
  grid-template-columns: 0.84fr 1.16fr;
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
  grid-template-columns: repeat(3, minmax(0, 1fr));
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
  font-size: 22px;
  color: #f3f9ff;
}

@media (max-width: 1200px) {
  .multi-grid,
  .grid,
  .summary-grid {
    grid-template-columns: 1fr;
  }
}
</style>
