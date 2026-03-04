<template>
  <section class="page">
    <h2>企业与岗位管理</h2>
    <p class="desc">
      本页已接入企业与岗位接口，可进行企业维护与岗位文本要求维护。
    </p>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel form-panel">
      <h3>{{ companyForm.id ? "编辑企业" : "新增企业" }}</h3>
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

    <div class="panel">
      <h3>企业列表</h3>
      <table>
        <thead>
          <tr>
            <th>ID</th>
            <th>企业名称</th>
            <th>领域方向</th>
            <th>城市</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in companies" :key="item.id">
            <td>{{ item.id }}</td>
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

    <div class="panel form-panel">
      <h3>{{ jobForm.id ? "编辑岗位" : "新增岗位" }}</h3>
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
          状态
          <select v-model="jobForm.status">
            <option value="open">开放</option>
            <option value="closed">关闭</option>
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

    <div class="panel">
      <h3>岗位列表</h3>
      <table>
        <thead>
          <tr>
            <th>ID</th>
            <th>岗位名称</th>
            <th>企业</th>
            <th>学历要求</th>
            <th>状态</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in jobs" :key="item.id">
            <td>{{ item.id }}</td>
            <td>{{ item.title }}</td>
            <td>{{ item.company_name }}</td>
            <td>{{ item.min_degree }}</td>
            <td>{{ item.status }}</td>
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
import { onMounted, reactive, ref } from "vue";
import api from "../services/api";

const companies = ref([]);
const jobs = ref([]);
const errorText = ref("");
const successText = ref("");

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
  required_tools_platform_text: "",
  status: "open"
});

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "操作失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
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
  jobForm.status = "open";
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
  jobForm.status = item.status || "open";
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
      required_tools_platform_text: jobForm.required_tools_platform_text,
      status: jobForm.status
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

async function initialize() {
  try {
    await loadCompanies();
    await loadJobs();
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
