<template>
  <section class="page gm-page fade-up">
    <header class="panel view-hero">
      <div>
        <p class="view-tag">Matching Lab</p>
        <h2>专业-岗位匹配分析</h2>
        <p class="desc">主流程聚焦“专业能力画像”与“社会岗位需求”之间的匹配度评估。</p>
      </div>
      <div class="hero-metrics">
        <article class="hero-card">
          <p>专业样本</p>
          <strong>{{ majors.length }}</strong>
        </article>
        <article class="hero-card">
          <p>岗位样本</p>
          <strong>{{ jobs.length }}</strong>
        </article>
        <article class="hero-card">
          <p>历史记录</p>
          <strong>{{ historyCount }}</strong>
        </article>
      </div>
    </header>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">单条匹配（专业 vs 岗位）</summary>
      <div class="panel-body">
        <div class="grid">
          <label>
            专业
            <select v-model.number="singleForm.major_id">
              <option :value="0">请选择专业</option>
              <option v-for="m in majors" :key="m.id" :value="m.id">{{ m.name }}（{{ m.code }}）</option>
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
            <input v-model.trim="singleForm.algorithm_version" placeholder="v0.2-major-job" />
          </label>
          <label>
            结果持久化
            <select v-model="singleForm.persist">
              <option :value="true">是</option>
              <option :value="false">否</option>
            </select>
          </label>
        </div>
        <div class="actions">
          <button :disabled="singleLoading" @click="runSingleMatch">
            {{ singleLoading ? "计算中..." : "执行单条匹配" }}
          </button>
        </div>

        <div v-if="singleResult" class="result-box">
        <p class="meta">
          专业：{{ majorNameById[singleResult.major_id] || singleResult.major_id }}
          | 岗位：{{ jobTitleById[singleResult.job_id] || singleResult.job_id }}
        </p>
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
    </details>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">批量匹配（单专业多岗位）</summary>
      <div class="panel-body">
        <div class="grid">
          <label>
            专业
            <select v-model.number="batchForm.major_id">
              <option :value="0">请选择专业</option>
              <option v-for="m in majors" :key="`batch-${m.id}`" :value="m.id">{{ m.name }}（{{ m.code }}）</option>
            </select>
          </label>
          <label>
            岗位 ID 列表（可选，逗号分隔）
            <input
              v-model.trim="batchForm.job_ids_text"
              placeholder="例如：1001,1002；为空时使用当前已加载岗位"
            />
          </label>
          <label>
            算法版本
            <input v-model.trim="batchForm.algorithm_version" placeholder="v0.2-major-job" />
          </label>
          <label>
            结果持久化
            <select v-model="batchForm.persist">
              <option :value="true">是</option>
              <option :value="false">否</option>
            </select>
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

        <h4>成功结果（按最终分降序）</h4>
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
            <tr v-for="(item, idx) in sortedBatchRows" :key="`batch-ok-${idx}`">
              <td>{{ idx + 1 }}</td>
              <td>{{ majorNameById[item.major_id] || item.major_id }}</td>
              <td>{{ jobTitleById[item.job_id] || item.job_id }}</td>
              <td>{{ item.final_score }}</td>
              <td>{{ item.semantic_score }}</td>
              <td>{{ item.skill_score }}</td>
              <td>{{ item.constraint_score }}</td>
            </tr>
            <tr v-if="!sortedBatchRows.length">
              <td colspan="7">暂无成功结果</td>
            </tr>
          </tbody>
        </table>

        <h4>失败结果</h4>
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
            <tr v-for="(item, idx) in batchFailed" :key="`batch-fail-${idx}`">
              <td>{{ item.major_id }}</td>
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
    </details>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">就业匹配重算（专业/学院/全量）</summary>
      <div class="panel-body">
        <div class="grid">
          <label>
            作用范围
            <select v-model="recomputeForm.scope">
              <option value="major">单专业</option>
              <option value="college">单学院</option>
              <option value="all">全量</option>
            </select>
          </label>
          <label v-if="recomputeForm.scope === 'major'">
            专业
            <select v-model.number="recomputeForm.major_id">
              <option :value="0">请选择专业</option>
              <option v-for="m in majors" :key="`recompute-major-${m.id}`" :value="m.id">{{ m.name }}（{{ m.code }}）</option>
            </select>
          </label>
          <label v-if="recomputeForm.scope === 'college'">
            学院
            <select v-model.number="recomputeForm.college_id">
              <option :value="0">请选择学院</option>
              <option v-for="c in colleges" :key="`recompute-college-${c.id}`" :value="c.id">{{ c.name }}</option>
            </select>
          </label>
          <label>
            算法版本
            <input v-model.trim="recomputeForm.algorithm_version" placeholder="v0.4-major-student" />
          </label>
          <label>
            写入匹配结果
            <select v-model="recomputeForm.persist">
              <option :value="true">是</option>
              <option :value="false">否</option>
            </select>
          </label>
          <label>
            最大处理条数
            <input v-model.number="recomputeForm.max_pairs" type="number" min="1" max="50000" />
          </label>
        </div>
        <div class="actions">
          <button :disabled="recomputeLoading" @click="runEmploymentRecompute">
            {{ recomputeLoading ? "重算中..." : "执行重算" }}
          </button>
        </div>
        <div v-if="recomputeSummary" class="result-box">
          <h4>重算摘要</h4>
          <p class="meta">
            范围：{{ recomputeSummary.scope }}，
            总量：{{ recomputeSummary.total_pairs || 0 }}，
            成功：{{ recomputeSummary.success_count || 0 }}，
            失败：{{ recomputeSummary.failed_count || 0 }}
          </p>
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
              <tr v-for="(item, idx) in recomputeRowsPreview" :key="`recompute-ok-${idx}`">
                <td>{{ item.student_id }}</td>
                <td>{{ item.major_id }}</td>
                <td>{{ item.job_id }}</td>
                <td>{{ item.final_score }}</td>
              </tr>
              <tr v-if="!recomputeRowsPreview.length">
                <td colspan="4">暂无成功记录</td>
              </tr>
            </tbody>
          </table>
          <h4>失败记录</h4>
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
              <tr v-for="(item, idx) in recomputeFailedPreview" :key="`recompute-fail-${idx}`">
                <td>{{ item.student_id }}</td>
                <td>{{ item.major_id }}</td>
                <td>{{ item.job_id }}</td>
                <td>{{ item.status }}</td>
                <td>{{ item.error }}</td>
              </tr>
              <tr v-if="!recomputeFailedPreview.length">
                <td colspan="5">无失败记录</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </details>

    <div class="panel">
      <h3>匹配历史</h3>
      <details class="form-panel collapsible-panel">
        <summary class="panel-summary">查询条件</summary>
        <div class="panel-body">
          <div class="grid">
            <label>
              专业
              <select v-model.number="historyForm.major_id">
                <option :value="0">全部专业</option>
                <option v-for="m in majors" :key="`history-major-${m.id}`" :value="m.id">{{ m.name }}（{{ m.code }}）</option>
              </select>
            </label>
            <label>
              岗位
              <select v-model.number="historyForm.job_id">
                <option :value="0">全部岗位</option>
                <option v-for="j in jobs" :key="`history-job-${j.id}`" :value="j.id">{{ j.title }}（{{ j.company_name }}）</option>
              </select>
            </label>
            <label>
              算法版本
              <input v-model.trim="historyForm.algorithm_version" placeholder="留空=全部" />
            </label>
            <label>
              运行模式
              <select v-model="historyForm.run_mode">
                <option value="">全部</option>
                <option value="single">single</option>
                <option value="batch">batch</option>
              </select>
            </label>
          </div>
          <div class="actions">
            <button :disabled="historyLoading" @click="searchHistory">
              {{ historyLoading ? "查询中..." : "查询历史" }}
            </button>
            <button class="secondary" :disabled="historyLoading" @click="resetHistoryFilters">重置条件</button>
          </div>
        </div>
      </details>
      <p class="meta">
        当前第 {{ historyPage }} / {{ historyTotalPages }} 页，共 {{ historyCount }} 条记录
      </p>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>专业</th>
            <th>岗位</th>
            <th>模式</th>
            <th>算法版本</th>
            <th>最终分</th>
            <th>生成时间</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in historyRows" :key="`history-${item.id}`">
            <td>{{ historyRowNo(index) }}</td>
            <td>{{ item.major_name }}</td>
            <td>{{ item.job_title }}（{{ item.company_name }}）</td>
            <td>{{ item.run_mode }}</td>
            <td>{{ item.algorithm_version }}</td>
            <td>{{ item.final_score }}</td>
            <td>{{ item.generated_at }}</td>
            <td>
              <button class="secondary" @click="removeHistoryItem(item)">删除</button>
            </td>
          </tr>
          <tr v-if="!historyRows.length">
            <td colspan="8">暂无匹配历史</td>
          </tr>
        </tbody>
      </table>
      <div class="actions">
        <button class="secondary" :disabled="historyLoading || historyPage <= 1" @click="prevHistoryPage">上一页</button>
        <button
          class="secondary"
          :disabled="historyLoading || historyPage >= historyTotalPages"
          @click="nextHistoryPage"
        >
          下一页
        </button>
      </div>
    </div>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import api from "../services/api";

const majors = ref([]);
const jobs = ref([]);
const colleges = ref([]);

const errorText = ref("");
const successText = ref("");
const singleLoading = ref(false);
const batchLoading = ref(false);
const recomputeLoading = ref(false);
const historyLoading = ref(false);

const singleResult = ref(null);
const batchRows = ref([]);
const batchFailed = ref([]);
const batchSummary = ref(null);
const recomputeRows = ref([]);
const recomputeFailed = ref([]);
const recomputeSummary = ref(null);
const historyRows = ref([]);
const historyCount = ref(0);
const historyPage = ref(1);
const historyPageSize = ref(20);

const singleForm = reactive({
  major_id: 0,
  job_id: 0,
  algorithm_version: "v0.2-major-job",
  persist: true
});

const batchForm = reactive({
  major_id: 0,
  job_ids_text: "",
  algorithm_version: "v0.2-major-job",
  persist: true
});

const recomputeForm = reactive({
  scope: "major",
  major_id: 0,
  college_id: 0,
  algorithm_version: "v0.4-major-student",
  persist: true,
  max_pairs: 20000
});

const historyForm = reactive({
  major_id: 0,
  job_id: 0,
  algorithm_version: "",
  run_mode: ""
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
    const companyName = item.company_name ? `（${item.company_name}）` : "";
    map[item.id] = `${item.title}${companyName}`;
  }
  return map;
});

const sortedBatchRows = computed(() => {
  return [...batchRows.value].sort((a, b) => {
    const scoreDiff = Number(b.final_score || 0) - Number(a.final_score || 0);
    if (Math.abs(scoreDiff) > 1e-9) {
      return scoreDiff;
    }
    return Number(a.job_id || 0) - Number(b.job_id || 0);
  });
});

const recomputeRowsPreview = computed(() => (recomputeRows.value || []).slice(0, 30));
const recomputeFailedPreview = computed(() => (recomputeFailed.value || []).slice(0, 30));

const historyTotalPages = computed(() => {
  const total = Number(historyCount.value || 0);
  const pageSize = Number(historyPageSize.value || 20);
  if (total <= 0) {
    return 1;
  }
  return Math.max(1, Math.ceil(total / pageSize));
});

function historyRowNo(index) {
  const page = Math.max(1, Number(historyPage.value || 1));
  const pageSize = Math.max(1, Number(historyPageSize.value || 20));
  return (page - 1) * pageSize + Number(index) + 1;
}

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

async function loadOptions() {
  const [majorResp, jobResp, collegeResp] = await Promise.all([
    api.get("/majors", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } }),
    api.get("/jobs", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } }),
    api.get("/colleges", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } })
  ]);
  majors.value = majorResp?.data?.data || [];
  jobs.value = jobResp?.data?.data || [];
  colleges.value = collegeResp?.data?.data || [];
}

async function loadHistory() {
  historyLoading.value = true;
  try {
    const params = {
      page: Number(historyPage.value || 1),
      page_size: Number(historyPageSize.value || 20),
      sort_by: "id",
      sort_order: "desc"
    };
    if (Number(historyForm.major_id || 0) > 0) {
      params.major_id = Number(historyForm.major_id);
    }
    if (Number(historyForm.job_id || 0) > 0) {
      params.job_id = Number(historyForm.job_id);
    }
    if (historyForm.algorithm_version.trim()) {
      params.algorithm_version = historyForm.algorithm_version.trim();
    }
    if (historyForm.run_mode.trim()) {
      params.run_mode = historyForm.run_mode.trim();
    }
    const resp = await api.get("/match/major-job/results", { params });
    historyRows.value = resp?.data?.data || [];
    historyCount.value = Number(resp?.data?.count || 0);
    const totalPages = Math.max(1, Math.ceil(historyCount.value / Number(historyPageSize.value || 20)));
    if (historyPage.value > totalPages) {
      historyPage.value = totalPages;
    }
  } catch (err) {
    setError(err);
  } finally {
    historyLoading.value = false;
  }
}

async function searchHistory() {
  historyPage.value = 1;
  await loadHistory();
}

async function resetHistoryFilters() {
  historyForm.major_id = 0;
  historyForm.job_id = 0;
  historyForm.algorithm_version = "";
  historyForm.run_mode = "";
  historyPage.value = 1;
  await loadHistory();
}

async function prevHistoryPage() {
  if (historyPage.value <= 1) {
    return;
  }
  historyPage.value -= 1;
  await loadHistory();
}

async function nextHistoryPage() {
  if (historyPage.value >= historyTotalPages.value) {
    return;
  }
  historyPage.value += 1;
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

async function runSingleMatch() {
  if (!singleForm.major_id || !singleForm.job_id) {
    setError(new Error("请先选择专业和岗位"));
    return;
  }
  singleLoading.value = true;
  try {
    const payload = {
      major_id: Number(singleForm.major_id),
      job_id: Number(singleForm.job_id),
      algorithm_version: singleForm.algorithm_version || "v0.2-major-job",
      persist: !!singleForm.persist
    };
    const resp = await api.post("/match/major-job", payload);
    singleResult.value = resp?.data?.data || null;
    setSuccess("单条匹配计算完成");
    if (singleForm.persist) {
      await loadHistory();
    }
  } catch (err) {
    setError(err);
  } finally {
    singleLoading.value = false;
  }
}

async function runBatchMatch() {
  if (!batchForm.major_id) {
    setError(new Error("请先选择专业"));
    return;
  }
  let jobIds = parseIdList(batchForm.job_ids_text);
  if (!jobIds.length) {
    jobIds = jobs.value.map((item) => Number(item.id)).filter((id) => Number.isInteger(id) && id > 0);
  }
  if (!jobIds.length) {
    setError(new Error("未找到可用于匹配的岗位ID"));
    return;
  }

  batchLoading.value = true;
  try {
    const payload = {
      major_ids: [Number(batchForm.major_id)],
      job_ids: jobIds,
      algorithm_version: batchForm.algorithm_version || "v0.2-major-job",
      persist: !!batchForm.persist
    };
    const resp = await api.post("/match/major-job/batch", payload);
    batchRows.value = resp?.data?.data || [];
    batchFailed.value = resp?.data?.failed || [];
    batchSummary.value = resp?.data?.summary || null;
    setSuccess("批量匹配计算完成");
    if (batchForm.persist) {
      await loadHistory();
    }
  } catch (err) {
    setError(err);
  } finally {
    batchLoading.value = false;
  }
}

async function runEmploymentRecompute() {
  const scope = String(recomputeForm.scope || "major");
  let url = "";
  if (scope === "major") {
    if (!Number(recomputeForm.major_id)) {
      setError(new Error("请选择专业"));
      return;
    }
    url = `/match/major/${Number(recomputeForm.major_id)}/recompute`;
  } else if (scope === "college") {
    if (!Number(recomputeForm.college_id)) {
      setError(new Error("请选择学院"));
      return;
    }
    url = `/match/college/${Number(recomputeForm.college_id)}/recompute`;
  } else {
    url = "/match/recompute-all";
  }

  recomputeLoading.value = true;
  try {
    const payload = {
      algorithm_version: recomputeForm.algorithm_version || "v0.4-major-student",
      persist: !!recomputeForm.persist,
      max_pairs: Math.max(1, Math.min(50000, Number(recomputeForm.max_pairs || 20000)))
    };
    const resp = await api.post(url, payload);
    recomputeRows.value = resp?.data?.data || [];
    recomputeFailed.value = resp?.data?.failed || [];
    recomputeSummary.value = resp?.data?.summary || null;
    setSuccess("就业匹配重算已完成");
    if (payload.persist) {
      await loadHistory();
    }
  } catch (err) {
    setError(err);
  } finally {
    recomputeLoading.value = false;
  }
}

async function initialize() {
  try {
    await loadOptions();
    await loadHistory();
  } catch (err) {
    setError(err);
  }
}

onMounted(initialize);
</script>

<style scoped>
.page {
  display: grid;
  gap: 12px;
  overflow: visible !important;
}

.view-hero {
  margin-top: 0;
  display: grid;
  gap: 12px;
  padding: 16px;
  border-radius: 18px;
  border: 1px solid rgba(173, 211, 255, 0.3);
  background:
    radial-gradient(circle at 8% 12%, rgba(56, 189, 248, 0.2), transparent 36%),
    linear-gradient(140deg, rgba(37, 99, 235, 0.2), rgba(15, 23, 42, 0.35));
}

.view-tag {
  margin: 0;
  display: inline-flex;
  align-items: center;
  padding: 2px 10px;
  border-radius: 999px;
  border: 1px solid rgba(147, 197, 253, 0.45);
  background: rgba(148, 163, 184, 0.2);
  color: #dbeafe;
  font-size: 12px;
}

h2 {
  margin: 8px 0 0;
}

.hero-metrics {
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.hero-card {
  border: 1px solid rgba(173, 211, 255, 0.24);
  border-radius: 14px;
  padding: 10px 12px;
  background: rgba(255, 255, 255, 0.1);
}

.hero-card p {
  margin: 0;
  color: #bcd3ee;
  font-size: 12px;
}

.hero-card strong {
  display: block;
  margin-top: 4px;
  font-size: 24px;
  color: #f2f8ff;
}

.desc {
  margin: 8px 0 0;
  color: #b5cae3;
}

.panel {
  margin-top: 0;
  padding: 12px;
  overflow-x: auto;
}

.form-panel {
  display: grid;
  gap: 10px;
}

.panel h3 {
  margin: 0 0 10px;
}

.collapsible-panel {
  padding: 0;
  overflow: hidden;
}

.panel-summary {
  list-style: none;
  cursor: pointer;
  font-weight: 600;
  padding: 12px;
  user-select: none;
  color: #d8e7f8;
}

.panel-summary::-webkit-details-marker {
  display: none;
}

.panel-summary::before {
  content: "▶";
  display: inline-block;
  margin-right: 8px;
  color: #8fb6dc;
  transform: rotate(0deg);
  transition: transform 0.15s ease;
}

.collapsible-panel[open] > .panel-summary::before {
  transform: rotate(90deg);
}

.panel-body {
  padding: 0 12px 12px;
}

.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(210px, 1fr));
  gap: 10px;
}

label {
  display: grid;
  gap: 6px;
  font-size: 13px;
  color: #d2e3f6;
}

input,
select,
textarea {
  border: 1px solid rgba(173, 211, 255, 0.32);
  border-radius: 10px;
  padding: 8px 10px;
  font: inherit;
  color: #f5f8ff;
  background: rgba(255, 255, 255, 0.08);
}

textarea {
  min-height: 70px;
}

.actions {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

button {
  border-radius: 10px;
}

.error {
  margin: 0;
}

.success {
  margin: 0;
}

.meta {
  color: #a8bfd9;
  font-size: 13px;
  margin: 4px 0;
}

.result-box {
  border-top: 1px dashed rgba(173, 211, 255, 0.26);
  margin-top: 10px;
  padding-top: 10px;
}

.score-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
  gap: 8px;
}

.score-card {
  border: 1px solid rgba(173, 211, 255, 0.26);
  border-radius: 12px;
  padding: 10px;
  background: rgba(255, 255, 255, 0.08);
}

.score-card h4 {
  margin: 0;
  font-size: 12px;
  color: #a8bfd9;
}

.score-card p {
  margin: 4px 0 0;
  font-size: 18px;
  font-weight: 700;
}

.score-card.highlight {
  border-color: rgba(56, 189, 248, 0.5);
  background: rgba(14, 165, 233, 0.2);
}

.split {
  margin-top: 8px;
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(190px, 1fr));
  gap: 10px;
}

.plain-list {
  margin: 0;
  padding-left: 18px;
  color: #d5e6f8;
}

table {
  width: 100%;
  border-collapse: collapse;
  margin-top: 8px;
  font-size: 13px;
  min-width: 900px;
}

th,
td {
  border: 1px solid rgba(173, 211, 255, 0.2);
  padding: 6px 8px;
  text-align: left;
  vertical-align: top;
  white-space: nowrap;
}

th {
  background: rgba(147, 197, 253, 0.14);
}

@media (max-width: 960px) {
  .hero-metrics {
    grid-template-columns: 1fr;
  }
}
</style>
