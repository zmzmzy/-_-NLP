<template>
  <section class="page gm-page fade-up">
    <h2>企业与岗位管理</h2>
    <p class="desc">
      本页已接入企业与岗位接口，可进行企业维护与岗位文本要求维护。
    </p>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">查询筛选</summary>
      <div class="panel-body">
        <div class="grid">
          <label>
            企业关键词
            <input v-model.trim="queryForm.company_keyword" placeholder="企业名称/主业/领域" />
          </label>
          <label>
            岗位关键词
            <input v-model.trim="queryForm.job_keyword" placeholder="岗位名称/企业/职责/技能" />
          </label>
          <label>
            岗位企业
            <select v-model.number="queryForm.job_company_id">
              <option :value="0">全部企业</option>
              <option v-for="c in companies" :key="`q-company-${c.id}`" :value="c.id">{{ c.name }}</option>
            </select>
          </label>
          <label>
            学历要求
            <select v-model="queryForm.job_min_degree">
              <option value="">全部</option>
              <option value="none">不限</option>
              <option value="diploma">专科</option>
              <option value="bachelor">本科</option>
              <option value="master">硕士</option>
              <option value="phd">博士</option>
            </select>
          </label>
          <label>
            岗位类型
            <select v-model="queryForm.job_type">
              <option value="">全部</option>
              <option value="full_time">全职</option>
              <option value="internship">实习</option>
              <option value="contract">合同制</option>
            </select>
          </label>
        </div>
        <div class="actions">
          <button class="secondary" @click="applyQueryFilters">查询</button>
          <button class="secondary" @click="resetQueryFilters">重置</button>
        </div>
        <p class="hint">
          企业结果 {{ filteredCompanies.length }}/{{ companies.length }}，
          岗位结果 {{ filteredJobs.length }}/{{ jobs.length }}
        </p>
      </div>
    </details>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">岗位导入（CSV 预检 + 应用）</summary>
      <div class="panel-body">
        <p class="hint">
          推荐表头：
          <code>company_name,title,job_type,min_degree,location_province,location_city,salary_range,responsibilities_text,technical_projects_text,required_knowledge_skills_text,required_tools_platform_text,published_at,domain_area,main_business,flagship_products,application_industries,company_province,company_city,company_website,company_description</code>
        </p>
        <div class="grid">
          <label>
            选择 CSV 文件
            <input type="file" accept=".csv,text/csv" @change="handleImportFileChange" />
          </label>
          <label>
            源文件名（可选）
            <input v-model.trim="importForm.source_filename" placeholder="如 jobs_2026.csv" />
          </label>
        </div>
        <label>
          CSV 文本
          <textarea
            v-model="importForm.csv_text"
            rows="6"
            placeholder="可直接粘贴 CSV 内容，或通过上方文件选择自动填充"
          ></textarea>
        </label>
        <div class="actions">
          <button :disabled="previewLoading" @click="previewJobImport">
            {{ previewLoading ? "预检中..." : "预检导入" }}
          </button>
          <button
            class="secondary"
            :disabled="applyLoading || !importBatchId"
            @click="applyJobImport"
          >
            {{ applyLoading ? "应用中..." : "确认应用" }}
          </button>
        </div>
        <p v-if="importSummary" class="hint">
          当前预检批次：#{{ importBatchId }}，总行 {{ importSummary.total_rows }}，
          有效 {{ importSummary.valid_rows }}，无效 {{ importSummary.invalid_rows }}，
          预计新增 {{ importSummary.estimated_inserted_rows }}，预计更新 {{ importSummary.estimated_updated_rows }}
        </p>
        <table v-if="importRows.length">
          <thead>
            <tr>
              <th>行号</th>
              <th>状态</th>
              <th>动作</th>
              <th>错误</th>
              <th>企业</th>
              <th>岗位</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="row in importRows" :key="row.row_no">
              <td>{{ row.row_no }}</td>
              <td>{{ row.row_status }}</td>
              <td>{{ row.plan_action || "-" }}</td>
              <td>{{ row.error_message || "-" }}</td>
              <td>{{ row.normalized_row?.company_name || "-" }}</td>
              <td>{{ row.normalized_row?.title || "-" }}</td>
            </tr>
          </tbody>
        </table>
      </div>
    </details>

    <details class="panel collapsible-panel">
      <summary class="panel-summary">岗位导入批次历史</summary>
      <div class="panel-body">
        <div class="actions">
          <button class="secondary" :disabled="historyLoading" @click="loadImportBatches">
            {{ historyLoading ? "刷新中..." : "刷新历史" }}
          </button>
        </div>
        <table>
          <thead>
            <tr>
              <th>序号</th>
              <th>批次ID</th>
              <th>状态</th>
              <th>来源</th>
              <th>总行</th>
              <th>有效</th>
              <th>无效</th>
              <th>新增</th>
              <th>更新</th>
              <th>触发人</th>
              <th>开始时间</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(item, index) in importBatches" :key="item.id">
              <td>{{ index + 1 }}</td>
              <td>{{ item.id }}</td>
              <td>{{ item.status }}</td>
              <td>{{ item.source_type }}</td>
              <td>{{ item.total_rows }}</td>
              <td>{{ item.valid_rows }}</td>
              <td>{{ item.invalid_rows }}</td>
              <td>{{ item.inserted_rows }}</td>
              <td>{{ item.updated_rows }}</td>
              <td>{{ item.triggered_by_username || item.triggered_by_user_id }}</td>
              <td>{{ item.started_at }}</td>
            </tr>
          </tbody>
        </table>
      </div>
    </details>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">{{ companyForm.id ? "编辑企业" : "新增企业" }}</summary>
      <div class="panel-body">
        <div class="grid">
          <label>
            企业名称
            <input v-model.trim="companyForm.name" placeholder="请输入企业名称" />
          </label>
          <label>
            领域方向
            <input v-model.trim="companyForm.domain_area" placeholder="如 工业互联网" />
          </label>
          <label>
            省份
            <input v-model.trim="companyForm.province" placeholder="可选" />
          </label>
          <label>
            城市
            <input v-model.trim="companyForm.city" placeholder="可选" />
          </label>
        </div>
        <label>
          企业主业
          <textarea v-model.trim="companyForm.main_business" rows="2"></textarea>
        </label>
        <label>
          拳头产品
          <textarea v-model.trim="companyForm.flagship_products" rows="2"></textarea>
        </label>
        <label>
          产品应用行业
          <textarea v-model.trim="companyForm.application_industries" rows="2"></textarea>
        </label>
        <div class="grid">
          <label>
            官网
            <input v-model.trim="companyForm.website" placeholder="可选" />
          </label>
          <label>
            描述
            <input v-model.trim="companyForm.description" placeholder="可选" />
          </label>
        </div>
        <div class="actions">
          <button @click="submitCompany">{{ companyForm.id ? "保存修改" : "新增企业" }}</button>
          <button class="secondary" @click="resetCompanyForm">清空</button>
        </div>
      </div>
    </details>

    <div class="panel">
      <h3>企业列表</h3>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>企业名称</th>
            <th>领域方向</th>
            <th>城市</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in filteredCompanies" :key="item.id">
            <td>{{ index + 1 }}</td>
            <td>{{ item.name }}</td>
            <td>{{ item.domain_area }}</td>
            <td>{{ item.province }} {{ item.city }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="editCompany(item)">编辑</button>
              <button class="small danger" @click="removeCompany(item)">删除</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">{{ jobForm.id ? "编辑岗位" : "新增岗位" }}</summary>
      <div class="panel-body">
        <div class="grid">
          <label>
            企业
            <select v-model.number="jobForm.company_id">
              <option :value="0">请选择企业</option>
              <option v-for="c in companies" :key="c.id" :value="c.id">{{ c.name }}</option>
            </select>
          </label>
          <label>
            岗位名称
            <input v-model.trim="jobForm.title" placeholder="如 C++后端开发工程师" />
          </label>
          <label>
            岗位类型
            <select v-model="jobForm.job_type">
              <option value="full_time">全职</option>
              <option value="internship">实习</option>
              <option value="contract">合同制</option>
            </select>
          </label>
          <label>
            最低学历
            <select v-model="jobForm.min_degree">
              <option value="none">不限</option>
              <option value="diploma">专科</option>
              <option value="bachelor">本科</option>
              <option value="master">硕士</option>
              <option value="phd">博士</option>
            </select>
          </label>
          <label>
            薪资范围
            <input v-model.trim="jobForm.salary_range" placeholder="可选" />
          </label>
          <label>
            省份
            <input v-model.trim="jobForm.location_province" placeholder="可选" />
          </label>
          <label>
            城市
            <input v-model.trim="jobForm.location_city" placeholder="可选" />
          </label>
        </div>
        <label>
          岗位职责描述
          <textarea v-model.trim="jobForm.responsibilities_text" rows="2"></textarea>
        </label>
        <label>
          技术项目描述
          <textarea v-model.trim="jobForm.technical_projects_text" rows="2"></textarea>
        </label>
        <label>
          专业知识与技能要求
          <textarea v-model.trim="jobForm.required_knowledge_skills_text" rows="2"></textarea>
        </label>
        <label>
          工具与平台要求
          <textarea v-model.trim="jobForm.required_tools_platform_text" rows="2"></textarea>
        </label>
        <div class="actions">
          <button @click="submitJob">{{ jobForm.id ? "保存修改" : "新增岗位" }}</button>
          <button class="secondary" @click="resetJobForm">清空</button>
        </div>
      </div>
    </details>

    <div class="panel">
      <h3>岗位列表</h3>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>岗位名称</th>
            <th>企业</th>
            <th>学历要求</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in filteredJobs" :key="item.id">
            <td>{{ index + 1 }}</td>
            <td>{{ item.title }}</td>
            <td>{{ item.company_name }}</td>
            <td>{{ item.min_degree }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="editJob(item)">编辑</button>
              <button class="small danger" @click="removeJob(item)">删除</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import api from "../services/api";

const companies = ref([]);
const jobs = ref([]);
const errorText = ref("");
const successText = ref("");
const importBatchId = ref(0);
const importSummary = ref(null);
const importRows = ref([]);
const importBatches = ref([]);
const previewLoading = ref(false);
const applyLoading = ref(false);
const historyLoading = ref(false);

const companyForm = reactive({
  id: 0,
  name: "",
  main_business: "",
  flagship_products: "",
  domain_area: "",
  application_industries: "",
  province: "",
  city: "",
  website: "",
  description: ""
});

const jobForm = reactive({
  id: 0,
  company_id: 0,
  title: "",
  job_type: "full_time",
  location_province: "",
  location_city: "",
  min_degree: "bachelor",
  salary_range: "",
  responsibilities_text: "",
  technical_projects_text: "",
  required_knowledge_skills_text: "",
  required_tools_platform_text: ""
});

const importForm = reactive({
  source_filename: "",
  csv_text: ""
});

const queryForm = reactive({
  company_keyword: "",
  job_keyword: "",
  job_company_id: 0,
  job_min_degree: "",
  job_type: ""
});

const activeQuery = reactive({
  company_keyword: "",
  job_keyword: "",
  job_company_id: 0,
  job_min_degree: "",
  job_type: ""
});

const filteredCompanies = computed(() => {
  const keyword = activeQuery.company_keyword.trim().toLowerCase();
  if (!keyword) {
    return companies.value;
  }
  return companies.value.filter((item) => {
    const searchText = `${item.name || ""} ${item.main_business || ""} ${item.domain_area || ""} ` +
      `${item.application_industries || ""} ${item.province || ""} ${item.city || ""}`.toLowerCase();
    return searchText.includes(keyword);
  });
});

const filteredJobs = computed(() => {
  const keyword = activeQuery.job_keyword.trim().toLowerCase();
  const companyId = Number(activeQuery.job_company_id || 0);
  const minDegree = activeQuery.job_min_degree.trim();
  const jobType = activeQuery.job_type.trim();

  return jobs.value.filter((item) => {
    if (companyId > 0 && Number(item.company_id || 0) !== companyId) {
      return false;
    }
    if (minDegree && String(item.min_degree || "") !== minDegree) {
      return false;
    }
    if (jobType && String(item.job_type || "") !== jobType) {
      return false;
    }
    if (!keyword) {
      return true;
    }
    const searchText = `${item.title || ""} ${item.company_name || ""} ${item.responsibilities_text || ""} ` +
      `${item.technical_projects_text || ""} ${item.required_knowledge_skills_text || ""} ` +
      `${item.required_tools_platform_text || ""}`.toLowerCase();
    return searchText.includes(keyword);
  });
});

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "操作失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

function applyQueryFilters() {
  activeQuery.company_keyword = queryForm.company_keyword;
  activeQuery.job_keyword = queryForm.job_keyword;
  activeQuery.job_company_id = Number(queryForm.job_company_id || 0);
  activeQuery.job_min_degree = queryForm.job_min_degree;
  activeQuery.job_type = queryForm.job_type;
}

function resetQueryFilters() {
  queryForm.company_keyword = "";
  queryForm.job_keyword = "";
  queryForm.job_company_id = 0;
  queryForm.job_min_degree = "";
  queryForm.job_type = "";
  applyQueryFilters();
}

function resetImportPreview() {
  importBatchId.value = 0;
  importSummary.value = null;
  importRows.value = [];
}

function resetCompanyForm() {
  companyForm.id = 0;
  companyForm.name = "";
  companyForm.main_business = "";
  companyForm.flagship_products = "";
  companyForm.domain_area = "";
  companyForm.application_industries = "";
  companyForm.province = "";
  companyForm.city = "";
  companyForm.website = "";
  companyForm.description = "";
}

function resetJobForm() {
  jobForm.id = 0;
  jobForm.company_id = 0;
  jobForm.title = "";
  jobForm.job_type = "full_time";
  jobForm.location_province = "";
  jobForm.location_city = "";
  jobForm.min_degree = "bachelor";
  jobForm.salary_range = "";
  jobForm.responsibilities_text = "";
  jobForm.technical_projects_text = "";
  jobForm.required_knowledge_skills_text = "";
  jobForm.required_tools_platform_text = "";
}

async function loadCompanies() {
  const resp = await api.get("/companies", {
    params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" }
  });
  companies.value = resp?.data?.data || [];
}

async function loadJobs() {
  const resp = await api.get("/jobs", {
    params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" }
  });
  jobs.value = resp?.data?.data || [];
}

function editCompany(item) {
  companyForm.id = item.id;
  companyForm.name = item.name || "";
  companyForm.main_business = item.main_business || "";
  companyForm.flagship_products = item.flagship_products || "";
  companyForm.domain_area = item.domain_area || "";
  companyForm.application_industries = item.application_industries || "";
  companyForm.province = item.province || "";
  companyForm.city = item.city || "";
  companyForm.website = item.website || "";
  companyForm.description = item.description || "";
}

async function submitCompany() {
  try {
    const payload = {
      name: companyForm.name,
      main_business: companyForm.main_business,
      flagship_products: companyForm.flagship_products,
      domain_area: companyForm.domain_area,
      application_industries: companyForm.application_industries,
      province: companyForm.province,
      city: companyForm.city,
      website: companyForm.website,
      description: companyForm.description
    };
    if (companyForm.id) {
      await api.put(`/companies/${companyForm.id}`, payload);
      setSuccess("企业修改成功");
    } else {
      await api.post("/companies", payload);
      setSuccess("企业新增成功");
    }
    resetCompanyForm();
    await loadCompanies();
    await loadJobs();
  } catch (err) {
    setError(err);
  }
}

async function removeCompany(item) {
  if (!window.confirm(`确认删除企业「${item.name}」吗？`)) {
    return;
  }
  try {
    await api.delete(`/companies/${item.id}`);
    setSuccess("企业删除成功");
    await loadCompanies();
    await loadJobs();
  } catch (err) {
    setError(err);
  }
}

function editJob(item) {
  jobForm.id = item.id;
  jobForm.company_id = Number(item.company_id || 0);
  jobForm.title = item.title || "";
  jobForm.job_type = item.job_type || "full_time";
  jobForm.location_province = item.location_province || "";
  jobForm.location_city = item.location_city || "";
  jobForm.min_degree = item.min_degree || "bachelor";
  jobForm.salary_range = item.salary_range || "";
  jobForm.responsibilities_text = item.responsibilities_text || "";
  jobForm.technical_projects_text = item.technical_projects_text || "";
  jobForm.required_knowledge_skills_text = item.required_knowledge_skills_text || "";
  jobForm.required_tools_platform_text = item.required_tools_platform_text || "";
}

async function submitJob() {
  try {
    const payload = {
      company_id: Number(jobForm.company_id),
      title: jobForm.title,
      job_type: jobForm.job_type,
      location_province: jobForm.location_province,
      location_city: jobForm.location_city,
      min_degree: jobForm.min_degree,
      salary_range: jobForm.salary_range,
      responsibilities_text: jobForm.responsibilities_text,
      technical_projects_text: jobForm.technical_projects_text,
      required_knowledge_skills_text: jobForm.required_knowledge_skills_text,
      required_tools_platform_text: jobForm.required_tools_platform_text
    };
    if (jobForm.id) {
      await api.put(`/jobs/${jobForm.id}`, payload);
      setSuccess("岗位修改成功");
    } else {
      await api.post("/jobs", payload);
      setSuccess("岗位新增成功");
    }
    resetJobForm();
    await loadJobs();
  } catch (err) {
    setError(err);
  }
}

async function removeJob(item) {
  if (!window.confirm(`确认删除岗位「${item.title}」吗？`)) {
    return;
  }
  try {
    await api.delete(`/jobs/${item.id}`);
    setSuccess("岗位删除成功");
    await loadJobs();
  } catch (err) {
    setError(err);
  }
}

async function handleImportFileChange(event) {
  const file = event?.target?.files?.[0];
  if (!file) {
    return;
  }
  importForm.source_filename = file.name || "";
  try {
    importForm.csv_text = await file.text();
  } catch (err) {
    setError(err);
  }
}

async function previewJobImport() {
  if (!importForm.csv_text.trim()) {
    setError(new Error("请先选择或粘贴 CSV 内容"));
    return;
  }
  previewLoading.value = true;
  try {
    const resp = await api.post("/jobs/import/preview", {
      source_type: "csv",
      source_filename: importForm.source_filename,
      csv_text: importForm.csv_text
    });
    const data = resp?.data?.data || {};
    importBatchId.value = Number(data.batch_id || 0);
    importSummary.value = data.summary || null;
    importRows.value = data.rows || [];
    setSuccess(`导入预检完成，批次 #${importBatchId.value}`);
    await loadImportBatches();
  } catch (err) {
    resetImportPreview();
    setError(err);
  } finally {
    previewLoading.value = false;
  }
}

async function applyJobImport() {
  if (!importBatchId.value) {
    setError(new Error("请先执行预检"));
    return;
  }
  applyLoading.value = true;
  try {
    const resp = await api.post("/jobs/import/apply", { batch_id: importBatchId.value });
    const result = resp?.data?.data || {};
    const inserted = Number(result.inserted_rows || 0);
    const updated = Number(result.updated_rows || 0);
    const skipped = Number(result.skipped_rows || 0);
    if (inserted === 0 && updated === 0) {
      setSuccess(
        `导入应用完成，批次 #${importBatchId.value} 未产生数据变更（跳过 ${skipped} 行）`
      );
    } else {
      setSuccess(
        `导入应用成功，批次 #${importBatchId.value}：新增 ${inserted}，更新 ${updated}，跳过 ${skipped}`
      );
    }
    await loadCompanies();
    await loadJobs();
    await loadImportBatches();
  } catch (err) {
    setError(err);
  } finally {
    applyLoading.value = false;
  }
}

async function loadImportBatches() {
  historyLoading.value = true;
  try {
    const resp = await api.get("/jobs/import/batches", {
      params: { page: 1, page_size: 20, sort_by: "id", sort_order: "desc" }
    });
    importBatches.value = resp?.data?.data || [];
  } catch (err) {
    setError(err);
  } finally {
    historyLoading.value = false;
  }
}

async function initialize() {
  try {
    await loadCompanies();
    await loadJobs();
    await loadImportBatches();
    applyQueryFilters();
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

.hint {
  margin: 0;
  color: #4c5a67;
  font-size: 12px;
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

.collapsible-panel {
  padding: 0;
}

.panel-summary {
  list-style: none;
  cursor: pointer;
  font-weight: 600;
  padding: 12px 14px;
  user-select: none;
}

.panel-summary::-webkit-details-marker {
  display: none;
}

.panel-summary::before {
  content: "▶";
  display: inline-block;
  margin-right: 8px;
  color: #5f86aa;
  transform: rotate(0deg);
  transition: transform 0.15s ease;
}

.collapsible-panel[open] > .panel-summary::before {
  transform: rotate(90deg);
}

.panel-body {
  padding: 0 14px 12px;
}

.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
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

.row-actions {
  white-space: nowrap;
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

.secondary {
  border-color: #5f86aa;
  background: #fff;
  color: #0d2b45;
}

.danger {
  border-color: #b42318;
  background: #fff;
  color: #b42318;
}

.small {
  padding: 4px 8px;
  font-size: 12px;
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
