<template>
  <section class="page">
    <h2>匹配分析</h2>
    <p class="desc">
      本页已接入单条匹配、批量匹配和历史结果查询，可直接查看匹配分与解释细节。
    </p>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel form-panel">
      <h3>单条匹配计算</h3>
      <div class="grid">
        <label>
          学生
          <select v-model.number="singleForm.student_id">
            <option :value="0">请选择学生</option>
            <option v-for="s in students" :key="s.id" :value="s.id">{{ s.full_name }}（{{ s.student_no }}）</option>
          </select>
        </label>
        <label>
          岗位
          <select v-model.number="singleForm.job_id">
            <option :value="0">请选择岗位</option>
            <option v-for="j in jobs" :key="j.id" :value="j.id">{{ j.title }}（{{ j.company_name }}）</option>
          </select>
        </label>
        <label>
          算法版本
          <input v-model.trim="singleForm.algorithm_version" placeholder="v0.1-baseline" />
        </label>
        <label>
          期望省份（可选）
          <input v-model.trim="singleForm.preferred_location_province" placeholder="如 江苏" />
        </label>
        <label>
          期望城市（可选）
          <input v-model.trim="singleForm.preferred_location_city" placeholder="如 苏州" />
        </label>
        <label class="checkbox">
          <input v-model="singleForm.persist" type="checkbox" />
          持久化写入 `match_results`
        </label>
      </div>
      <div class="actions">
        <button :disabled="singleLoading" @click="runSingleMatch">
          {{ singleLoading ? "计算中..." : "执行单条匹配" }}
        </button>
      </div>

      <div v-if="singleResult" class="result-box">
        <div class="score-grid">
          <article class="score-card">
            <h4>语义分</h4>
            <p>{{ singleResult.semantic_score }}</p>
          </article>
          <article class="score-card">
            <h4>技能分</h4>
            <p>{{ singleResult.skill_score }}</p>
          </article>
          <article class="score-card">
            <h4>约束分</h4>
            <p>{{ singleResult.constraint_score }}</p>
          </article>
          <article class="score-card highlight">
            <h4>最终分</h4>
            <p>{{ singleResult.final_score }}</p>
          </article>
        </div>

        <div class="split">
          <div>
            <h4>命中技能</h4>
            <ul class="plain-list">
              <li v-for="(item, idx) in singleResult.matched_skills || []" :key="`single-m-${idx}`">
                {{ item.skill_name }}（岗位权重 {{ item.job_weight }}）
              </li>
              <li v-if="!(singleResult.matched_skills || []).length">无</li>
            </ul>
          </div>
          <div>
            <h4>缺失技能</h4>
            <ul class="plain-list">
              <li v-for="(item, idx) in singleResult.missing_skills || []" :key="`single-miss-${idx}`">
                {{ item.skill_name }}<span v-if="item.mandatory">（必备）</span>
              </li>
              <li v-if="!(singleResult.missing_skills || []).length">无</li>
            </ul>
          </div>
          <div>
            <h4>约束失败项</h4>
            <ul class="plain-list">
              <li v-for="(item, idx) in singleResult.constraint_failures || []" :key="`single-c-${idx}`">
                {{ item.type }}：{{ item.message }}
              </li>
              <li v-if="!(singleResult.constraint_failures || []).length">无</li>
            </ul>
          </div>
        </div>
      </div>
    </div>

    <div class="panel form-panel">
      <h3>批量匹配计算</h3>
      <div class="grid">
        <label>
          学生 ID 列表（逗号分隔）
          <input v-model.trim="batchForm.student_ids_text" placeholder="如 5001,5002" />
        </label>
        <label>
          岗位 ID 列表（逗号分隔）
          <input v-model.trim="batchForm.job_ids_text" placeholder="如 4001,4002" />
        </label>
        <label>
          算法版本
          <input v-model.trim="batchForm.algorithm_version" placeholder="v0.1-baseline" />
        </label>
        <label>
          期望省份（可选）
          <input v-model.trim="batchForm.preferred_location_province" />
        </label>
        <label>
          期望城市（可选）
          <input v-model.trim="batchForm.preferred_location_city" />
        </label>
        <label class="checkbox">
          <input v-model="batchForm.persist" type="checkbox" />
          持久化写入 `match_results`
        </label>
      </div>
      <div class="actions">
        <button :disabled="batchLoading" @click="runBatchMatch">
          {{ batchLoading ? "计算中..." : "执行批量匹配" }}
        </button>
      </div>

      <div v-if="batchSummary" class="result-box">
        <h4>批量结果汇总</h4>
        <p class="meta">
          总组合：{{ batchSummary.total_pairs }}，成功：{{ batchSummary.success_count }}，失败：{{ batchSummary.failed_count }}
        </p>

        <h4>成功结果</h4>
        <table>
          <thead>
            <tr>
              <th>学生ID</th>
              <th>岗位ID</th>
              <th>最终分</th>
              <th>语义分</th>
              <th>技能分</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(item, idx) in batchData" :key="`batch-ok-${idx}`">
              <td>{{ item.student_id }}</td>
              <td>{{ item.job_id }}</td>
              <td>{{ item.final_score }}</td>
              <td>{{ item.semantic_score }}</td>
              <td>{{ item.skill_score }}</td>
            </tr>
            <tr v-if="!batchData.length">
              <td colspan="5">暂无成功结果</td>
            </tr>
          </tbody>
        </table>

        <h4>失败结果</h4>
        <table>
          <thead>
            <tr>
              <th>学生ID</th>
              <th>岗位ID</th>
              <th>状态码</th>
              <th>错误信息</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(item, idx) in batchFailed" :key="`batch-fail-${idx}`">
              <td>{{ item.student_id }}</td>
              <td>{{ item.job_id }}</td>
              <td>{{ item.status }}</td>
              <td>{{ item.error }}</td>
            </tr>
            <tr v-if="!batchFailed.length">
              <td colspan="4">暂无失败结果</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div class="panel form-panel">
      <h3>历史结果查询</h3>
      <div class="grid">
        <label>
          学生（可选）
          <select v-model.number="historyQuery.student_id">
            <option :value="0">全部</option>
            <option v-for="s in students" :key="`h-s-${s.id}`" :value="s.id">{{ s.full_name }}（{{ s.student_no }}）</option>
          </select>
        </label>
        <label>
          专业（可选）
          <select v-model.number="historyQuery.major_id">
            <option :value="0">全部</option>
            <option v-for="m in majors" :key="`h-m-${m.id}`" :value="m.id">{{ m.name }}</option>
          </select>
        </label>
        <label>
          岗位（可选）
          <select v-model.number="historyQuery.job_id">
            <option :value="0">全部</option>
            <option v-for="j in jobs" :key="`h-j-${j.id}`" :value="j.id">{{ j.title }}</option>
          </select>
        </label>
        <label>
          算法版本（可选）
          <input v-model.trim="historyQuery.algorithm_version" placeholder="v0.1-baseline" />
        </label>
        <label>
          排序字段
          <select v-model="historyQuery.sort_by">
            <option value="id">id</option>
            <option value="final_score">final_score</option>
            <option value="semantic_score">semantic_score</option>
            <option value="skill_score">skill_score</option>
            <option value="constraint_score">constraint_score</option>
            <option value="generated_at">generated_at</option>
          </select>
        </label>
        <label>
          排序方向
          <select v-model="historyQuery.sort_order">
            <option value="desc">desc</option>
            <option value="asc">asc</option>
          </select>
        </label>
      </div>
      <div class="actions">
        <button :disabled="historyLoading" @click="loadHistoryResults(1)">
          {{ historyLoading ? "查询中..." : "查询历史结果" }}
        </button>
        <button class="secondary" @click="resetHistoryQuery">重置筛选</button>
      </div>

      <table>
        <thead>
          <tr>
            <th>ID</th>
            <th>学生</th>
            <th>专业</th>
            <th>岗位</th>
            <th>最终分</th>
            <th>生成时间</th>
            <th>详情</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in historyRows" :key="item.id">
            <td>{{ item.id }}</td>
            <td>{{ item.student_name }}</td>
            <td>{{ item.major_name }}</td>
            <td>{{ item.job_title }}</td>
            <td>{{ item.final_score }}</td>
            <td>{{ item.generated_at }}</td>
            <td>
              <button class="small secondary" @click="toggleHistoryDetail(item.id)">
                {{ detailRowId === item.id ? "收起" : "查看" }}
              </button>
            </td>
          </tr>
          <tr v-if="!historyRows.length">
            <td colspan="7">暂无数据</td>
          </tr>
        </tbody>
      </table>

      <p class="meta">
        共 {{ historyMeta.count }} 条，当前第 {{ historyMeta.page }} 页 / 共 {{ historyMeta.total_pages }} 页
      </p>
      <div class="actions">
        <button class="secondary" :disabled="historyMeta.page <= 1" @click="loadHistoryResults(historyMeta.page - 1)">
          上一页
        </button>
        <button
          class="secondary"
          :disabled="historyMeta.page >= historyMeta.total_pages || historyMeta.total_pages === 0"
          @click="loadHistoryResults(historyMeta.page + 1)"
        >
          下一页
        </button>
      </div>

      <div v-if="detailRow" class="result-box">
        <h4>结果详情：#{{ detailRow.id }}</h4>
        <div class="score-grid">
          <article class="score-card">
            <h4>语义分</h4>
            <p>{{ detailRow.semantic_score }}</p>
          </article>
          <article class="score-card">
            <h4>技能分</h4>
            <p>{{ detailRow.skill_score }}</p>
          </article>
          <article class="score-card">
            <h4>约束分</h4>
            <p>{{ detailRow.constraint_score }}</p>
          </article>
          <article class="score-card highlight">
            <h4>最终分</h4>
            <p>{{ detailRow.final_score }}</p>
          </article>
        </div>
        <div class="split">
          <div>
            <h4>命中技能</h4>
            <ul class="plain-list">
              <li v-for="(item, idx) in detailRow.matched_skills || []" :key="`history-m-${idx}`">
                {{ item.skill_name }}
              </li>
              <li v-if="!(detailRow.matched_skills || []).length">无</li>
            </ul>
          </div>
          <div>
            <h4>缺失技能</h4>
            <ul class="plain-list">
              <li v-for="(item, idx) in detailRow.missing_skills || []" :key="`history-miss-${idx}`">
                {{ item.skill_name }}<span v-if="item.mandatory">（必备）</span>
              </li>
              <li v-if="!(detailRow.missing_skills || []).length">无</li>
            </ul>
          </div>
          <div>
            <h4>约束失败项</h4>
            <ul class="plain-list">
              <li v-for="(item, idx) in detailRow.constraint_failures || []" :key="`history-c-${idx}`">
                {{ item.type }}：{{ item.message }}
              </li>
              <li v-if="!(detailRow.constraint_failures || []).length">无</li>
            </ul>
          </div>
        </div>
      </div>
    </div>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import api from "../services/api";

const students = ref([]);
const jobs = ref([]);
const majors = ref([]);

const errorText = ref("");
const successText = ref("");
const singleLoading = ref(false);
const batchLoading = ref(false);
const historyLoading = ref(false);

const singleResult = ref(null);
const batchData = ref([]);
const batchFailed = ref([]);
const batchSummary = ref(null);
const historyRows = ref([]);
const detailRowId = ref(0);

const historyMeta = reactive({
  count: 0,
  page: 1,
  page_size: 20,
  total_pages: 0
});

const singleForm = reactive({
  student_id: 0,
  job_id: 0,
  algorithm_version: "v0.1-baseline",
  persist: true,
  preferred_location_province: "",
  preferred_location_city: ""
});

const batchForm = reactive({
  student_ids_text: "",
  job_ids_text: "",
  algorithm_version: "v0.1-baseline",
  persist: false,
  preferred_location_province: "",
  preferred_location_city: ""
});

const historyQuery = reactive({
  page: 1,
  page_size: 20,
  sort_by: "id",
  sort_order: "desc",
  student_id: 0,
  major_id: 0,
  job_id: 0,
  algorithm_version: ""
});

const detailRow = computed(() => historyRows.value.find((row) => row.id === detailRowId.value) || null);

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

function buildMatchPayload(form) {
  const payload = {
    algorithm_version: form.algorithm_version || "v0.1-baseline",
    persist: !!form.persist
  };
  if (form.preferred_location_province) {
    payload.preferred_location_province = form.preferred_location_province;
  }
  if (form.preferred_location_city) {
    payload.preferred_location_city = form.preferred_location_city;
  }
  return payload;
}

async function loadOptions() {
  const [studentResp, jobResp, majorResp] = await Promise.all([
    api.get("/students", { params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" } }),
    api.get("/jobs", { params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" } }),
    api.get("/majors", { params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" } })
  ]);
  students.value = studentResp?.data?.data || [];
  jobs.value = jobResp?.data?.data || [];
  majors.value = majorResp?.data?.data || [];
}

async function runSingleMatch() {
  if (!singleForm.student_id || !singleForm.job_id) {
    setError(new Error("请先选择学生和岗位"));
    return;
  }
  singleLoading.value = true;
  try {
    const payload = {
      ...buildMatchPayload(singleForm),
      student_id: Number(singleForm.student_id),
      job_id: Number(singleForm.job_id)
    };
    const resp = await api.post("/match/single", payload);
    singleResult.value = resp?.data?.data || null;
    setSuccess("单条匹配计算完成");
    if (singleForm.persist) {
      await loadHistoryResults(1);
    }
  } catch (err) {
    setError(err);
  } finally {
    singleLoading.value = false;
  }
}

async function runBatchMatch() {
  const studentIds = parseIdList(batchForm.student_ids_text);
  const jobIds = parseIdList(batchForm.job_ids_text);
  if (!studentIds.length || !jobIds.length) {
    setError(new Error("请填写学生ID列表和岗位ID列表"));
    return;
  }
  batchLoading.value = true;
  try {
    const payload = {
      ...buildMatchPayload(batchForm),
      student_ids: studentIds,
      job_ids: jobIds
    };
    const resp = await api.post("/match/batch", payload);
    batchData.value = resp?.data?.data || [];
    batchFailed.value = resp?.data?.failed || [];
    batchSummary.value = resp?.data?.summary || null;
    setSuccess("批量匹配计算完成");
    if (batchForm.persist) {
      await loadHistoryResults(1);
    }
  } catch (err) {
    setError(err);
  } finally {
    batchLoading.value = false;
  }
}

function buildHistoryParams(targetPage) {
  const params = {
    page: targetPage,
    page_size: historyQuery.page_size,
    sort_by: historyQuery.sort_by,
    sort_order: historyQuery.sort_order
  };
  if (historyQuery.student_id) {
    params.student_id = historyQuery.student_id;
  }
  if (historyQuery.major_id) {
    params.major_id = historyQuery.major_id;
  }
  if (historyQuery.job_id) {
    params.job_id = historyQuery.job_id;
  }
  if (historyQuery.algorithm_version) {
    params.algorithm_version = historyQuery.algorithm_version;
  }
  return params;
}

async function loadHistoryResults(targetPage = historyQuery.page) {
  historyLoading.value = true;
  try {
    const resp = await api.get("/match-results", { params: buildHistoryParams(targetPage) });
    historyRows.value = resp?.data?.data || [];
    historyMeta.count = Number(resp?.data?.count || 0);
    historyMeta.page = Number(resp?.data?.page || targetPage);
    historyMeta.page_size = Number(resp?.data?.page_size || historyQuery.page_size);
    historyMeta.total_pages = Number(resp?.data?.total_pages || 0);
    historyQuery.page = historyMeta.page;
    if (!historyRows.value.find((row) => row.id === detailRowId.value)) {
      detailRowId.value = 0;
    }
  } catch (err) {
    setError(err);
  } finally {
    historyLoading.value = false;
  }
}

function toggleHistoryDetail(id) {
  detailRowId.value = detailRowId.value === id ? 0 : id;
}

function resetHistoryQuery() {
  historyQuery.page = 1;
  historyQuery.sort_by = "id";
  historyQuery.sort_order = "desc";
  historyQuery.student_id = 0;
  historyQuery.major_id = 0;
  historyQuery.job_id = 0;
  historyQuery.algorithm_version = "";
  detailRowId.value = 0;
  loadHistoryResults(1);
}

async function initialize() {
  try {
    await loadOptions();
    await loadHistoryResults(1);
  } catch (err) {
    setError(err);
  }
}

onMounted(initialize);
</script>

<style scoped>
.page {
  background: #fff;
  padding: 20px;
  border-radius: 10px;
}

.desc {
  color: #4c5a67;
}

.panel {
  margin-top: 14px;
  border: 1px solid #dfe7ef;
  border-radius: 8px;
  padding: 12px 14px;
  background: #fafcff;
}

.form-panel {
  display: grid;
  gap: 10px;
}

.panel h3 {
  margin: 0 0 8px;
}

.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(190px, 1fr));
  gap: 10px;
}

label {
  display: grid;
  gap: 6px;
  font-size: 13px;
}

input,
select,
textarea {
  border: 1px solid #c9d6e3;
  border-radius: 6px;
  padding: 8px 10px;
  font-size: 13px;
  font-family: inherit;
}

input[type="checkbox"] {
  width: 14px;
  height: 14px;
}

.checkbox {
  display: flex;
  align-items: center;
  gap: 8px;
}

.actions {
  display: flex;
  gap: 8px;
}

button {
  border: 1px solid #0d2b45;
  background: #0d2b45;
  color: #fff;
  border-radius: 6px;
  padding: 7px 12px;
  cursor: pointer;
}

button:disabled {
  opacity: 0.65;
  cursor: not-allowed;
}

.secondary {
  border-color: #5f86aa;
  background: #fff;
  color: #0d2b45;
}

.small {
  padding: 4px 8px;
  font-size: 12px;
}

table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

th,
td {
  border-bottom: 1px solid #e6edf4;
  padding: 8px 6px;
  text-align: left;
}

.meta {
  margin: 8px 0;
  color: #4c5a67;
}

.result-box {
  border: 1px dashed #c9d6e3;
  border-radius: 8px;
  background: #fff;
  padding: 12px;
}

.score-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
  gap: 8px;
}

.score-card {
  border: 1px solid #dfe7ef;
  border-radius: 8px;
  padding: 8px;
  background: #fafcff;
}

.score-card h4 {
  margin: 0;
  font-size: 12px;
  color: #4c5a67;
}

.score-card p {
  margin: 4px 0 0;
  font-weight: 700;
  font-size: 18px;
}

.score-card.highlight {
  border-color: #0d2b45;
}

.split {
  margin-top: 10px;
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  gap: 10px;
}

.plain-list {
  margin: 0;
  padding-left: 18px;
  font-size: 13px;
}

.plain-list li {
  margin: 4px 0;
}

.error {
  margin: 6px 0 0;
  color: #b42318;
}

.success {
  margin: 6px 0 0;
  color: #027a48;
}
</style>
