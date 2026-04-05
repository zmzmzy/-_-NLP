<template>
  <section class="jc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel filter-panel">
      <div class="grid">
        <label>
          关键词
          <input v-model.trim="queryForm.keyword" placeholder="岗位名称/企业/职责/技能" @keyup.enter="search" />
        </label>
        <label>
          企业
          <select v-model.number="queryForm.company_id">
            <option :value="0">全部企业</option>
            <option v-for="c in companies" :key="`q-company-${c.id}`" :value="c.id">{{ c.name }}</option>
          </select>
        </label>
        <label>
          学历要求
          <select v-model="queryForm.min_degree">
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
        <button class="secondary" :disabled="loading" @click="search">查询</button>
        <button class="secondary" :disabled="loading" @click="resetSearch">重置</button>
        <router-link class="jc-link-btn secondary" :to="{ name: 'job-center-imports' }">去导入中心</router-link>
      </div>
    </div>

    <div class="panel">
      <div class="section-head">
        <h3>岗位列表</h3>
        <p class="meta">共 {{ pager.total_count }} 条，当前第 {{ pager.page }} / {{ pager.total_pages }} 页</p>
      </div>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>岗位名称</th>
            <th>企业</th>
            <th>岗位类型</th>
            <th>学历要求</th>
            <th>城市</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in jobs" :key="item.id">
            <td>{{ rowNo(index) }}</td>
            <td>{{ item.title }}</td>
            <td>{{ item.company_name }}</td>
            <td>{{ item.job_type }}</td>
            <td>{{ item.min_degree }}</td>
            <td>{{ item.location_province || '-' }} {{ item.location_city || '' }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="editJob(item)">编辑</button>
              <button class="small danger" @click="removeJob(item)">删除</button>
            </td>
          </tr>
          <tr v-if="!jobs.length">
            <td colspan="7">暂无岗位数据</td>
          </tr>
        </tbody>
      </table>
      <div class="actions page-actions">
        <button class="secondary" :disabled="loading || pager.page <= 1" @click="loadJobs(pager.page - 1)">上一页</button>
        <button class="secondary" :disabled="loading || pager.page >= pager.total_pages" @click="loadJobs(pager.page + 1)">下一页</button>
      </div>
    </div>

    <div class="panel form-panel">
      <h3>{{ jobForm.id ? "编辑岗位" : "新增岗位" }}</h3>
      <div class="grid">
        <label>
          企业
          <select v-model.number="jobForm.company_id">
            <option :value="0">请选择企业</option>
            <option v-for="c in companies" :key="`f-company-${c.id}`" :value="c.id">{{ c.name }}</option>
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
        <button :disabled="submitLoading" @click="submitJob">{{ submitLoading ? "提交中..." : (jobForm.id ? "保存修改" : "新增岗位") }}</button>
        <button class="secondary" :disabled="submitLoading" @click="resetJobForm">清空</button>
      </div>
    </div>
  </section>
</template>

<script setup>
import { onMounted, reactive, ref, watch } from "vue";
import { useRoute, useRouter } from "vue-router";
import api from "../../services/api";

const route = useRoute();
const router = useRouter();

const companies = ref([]);
const jobs = ref([]);

const loading = ref(false);
const submitLoading = ref(false);
const errorText = ref("");
const successText = ref("");

const queryForm = reactive({
  keyword: "",
  company_id: 0,
  min_degree: "",
  job_type: ""
});

const pager = reactive({
  page: 1,
  page_size: 20,
  total_count: 0,
  total_pages: 1
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

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "操作失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

function rowNo(index) {
  return (Math.max(1, Number(pager.page || 1)) - 1) * Math.max(1, Number(pager.page_size || 20)) + Number(index) + 1;
}

function syncRouteCompanyFilter() {
  const companyId = Number(queryForm.company_id || 0);
  const nextQuery = { ...route.query };
  if (companyId > 0) {
    nextQuery.company_id = String(companyId);
  } else {
    delete nextQuery.company_id;
  }
  router.replace({ query: nextQuery });
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
    params: { page: 1, page_size: 400, sort_by: "id", sort_order: "desc" }
  });
  companies.value = resp?.data?.data || [];
}

async function loadJobs(page = 1) {
  loading.value = true;
  try {
    const params = {
      page: Number(page || 1),
      page_size: Number(pager.page_size || 20),
      sort_by: "id",
      sort_order: "desc"
    };
    if (queryForm.keyword.trim()) {
      params.keyword = queryForm.keyword.trim();
    }
    if (Number(queryForm.company_id || 0) > 0) {
      params.company_id = Number(queryForm.company_id);
    }
    if (queryForm.min_degree.trim()) {
      params.min_degree = queryForm.min_degree.trim();
    }
    if (queryForm.job_type.trim()) {
      params.job_type = queryForm.job_type.trim();
    }

    const resp = await api.get("/jobs", { params });
    jobs.value = resp?.data?.data || [];
    pager.page = Number(resp?.data?.page || params.page);
    pager.page_size = Number(resp?.data?.page_size || params.page_size);
    pager.total_count = Number(resp?.data?.count || 0);
    pager.total_pages = Number(resp?.data?.total_pages || 1);
    syncRouteCompanyFilter();
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

function search() {
  loadJobs(1);
}

function resetSearch() {
  queryForm.keyword = "";
  queryForm.company_id = 0;
  queryForm.min_degree = "";
  queryForm.job_type = "";
  loadJobs(1);
}

function editJob(item) {
  jobForm.id = Number(item.id || 0);
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
  if (!jobForm.company_id || !jobForm.title.trim()) {
    setError(new Error("企业和岗位名称不能为空"));
    return;
  }

  submitLoading.value = true;
  try {
    const payload = {
      company_id: Number(jobForm.company_id),
      title: jobForm.title.trim(),
      job_type: jobForm.job_type,
      location_province: jobForm.location_province.trim(),
      location_city: jobForm.location_city.trim(),
      min_degree: jobForm.min_degree,
      salary_range: jobForm.salary_range.trim(),
      responsibilities_text: jobForm.responsibilities_text.trim() || "待补充",
      technical_projects_text: jobForm.technical_projects_text.trim() || "待补充",
      required_knowledge_skills_text: jobForm.required_knowledge_skills_text.trim() || "待补充",
      required_tools_platform_text: jobForm.required_tools_platform_text.trim() || "待补充"
    };
    if (jobForm.id) {
      await api.put(`/jobs/${jobForm.id}`, payload);
      setSuccess("岗位修改成功");
    } else {
      await api.post("/jobs", payload);
      setSuccess("岗位新增成功");
    }
    resetJobForm();
    await loadJobs(1);
  } catch (err) {
    setError(err);
  } finally {
    submitLoading.value = false;
  }
}

async function removeJob(item) {
  if (!window.confirm(`确认删除岗位「${item.title}」吗？`)) {
    return;
  }
  try {
    await api.delete(`/jobs/${item.id}`);
    setSuccess("岗位删除成功");
    await loadJobs(pager.page);
  } catch (err) {
    setError(err);
  }
}

watch(
  () => route.query.company_id,
  (val) => {
    const q = Number(val || 0);
    if (q >= 0 && q !== Number(queryForm.company_id || 0)) {
      queryForm.company_id = q;
      loadJobs(1);
    }
  }
);

onMounted(async () => {
  try {
    await loadCompanies();
    queryForm.company_id = Number(route.query.company_id || 0);
    await loadJobs(1);
  } catch (err) {
    setError(err);
  }
});
</script>

<style scoped>
.jc-view {
  display: grid;
  gap: 12px;
}

.filter-panel,
.form-panel {
  display: grid;
  gap: 10px;
}

.section-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.section-head h3 {
  margin: 0;
}

.page-actions {
  margin-top: 10px;
}

.jc-link-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  text-decoration: none;
  border-radius: 12px;
  border: 1px solid rgba(147, 197, 253, 0.34);
  background: rgba(255, 255, 255, 0.11);
  color: #f8fbff;
  padding: 8px 12px;
}

.jc-link-btn.secondary {
  border-color: rgba(147, 197, 253, 0.34);
  background: rgba(255, 255, 255, 0.11);
}
</style>
