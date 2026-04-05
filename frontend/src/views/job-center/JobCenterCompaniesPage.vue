<template>
  <section class="jc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel filter-panel">
      <div class="grid">
        <label>
          关键词
          <input v-model.trim="queryForm.keyword" placeholder="企业名称/主业/领域/描述" @keyup.enter="search" />
        </label>
        <label>
          领域方向
          <input v-model.trim="queryForm.domain_area" placeholder="如 工业互联网" @keyup.enter="search" />
        </label>
        <label>
          省份
          <input v-model.trim="queryForm.province" placeholder="如 江苏" @keyup.enter="search" />
        </label>
        <label>
          城市
          <input v-model.trim="queryForm.city" placeholder="如 南京" @keyup.enter="search" />
        </label>
      </div>
      <div class="actions">
        <button class="secondary" :disabled="loading" @click="search">查询</button>
        <button class="secondary" :disabled="loading" @click="resetSearch">重置</button>
        <button @click="resetCompanyForm">新增企业</button>
      </div>
    </div>

    <div class="panel">
      <div class="section-head">
        <h3>企业列表</h3>
        <p class="meta">共 {{ pager.total_count }} 条，当前第 {{ pager.page }} / {{ pager.total_pages }} 页</p>
      </div>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>企业名称</th>
            <th>领域方向</th>
            <th>主业</th>
            <th>城市</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in companies" :key="item.id">
            <td>{{ rowNo(index) }}</td>
            <td>{{ item.name }}</td>
            <td>{{ item.domain_area }}</td>
            <td>{{ item.main_business || '-' }}</td>
            <td>{{ item.province || '-' }} {{ item.city || '' }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="openCompanyJobs(item)">下属岗位</button>
              <button class="small secondary" @click="editCompany(item)">编辑</button>
              <button class="small danger" @click="removeCompany(item)">删除</button>
            </td>
          </tr>
          <tr v-if="!companies.length">
            <td colspan="6">暂无企业数据</td>
          </tr>
        </tbody>
      </table>
      <div class="actions page-actions">
        <button class="secondary" :disabled="loading || pager.page <= 1" @click="loadCompanies(pager.page - 1)">上一页</button>
        <button class="secondary" :disabled="loading || pager.page >= pager.total_pages" @click="loadCompanies(pager.page + 1)">下一页</button>
      </div>
    </div>

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
        <button :disabled="submitLoading" @click="submitCompany">{{ submitLoading ? "提交中..." : (companyForm.id ? "保存修改" : "新增企业") }}</button>
        <button class="secondary" :disabled="submitLoading" @click="resetCompanyForm">清空</button>
      </div>
    </div>
  </section>
</template>

<script setup>
import { onMounted, reactive, ref } from "vue";
import { useRouter } from "vue-router";
import api from "../../services/api";

const router = useRouter();

const companies = ref([]);
const loading = ref(false);
const submitLoading = ref(false);
const errorText = ref("");
const successText = ref("");

const queryForm = reactive({
  keyword: "",
  domain_area: "",
  province: "",
  city: ""
});

const pager = reactive({
  page: 1,
  page_size: 20,
  total_count: 0,
  total_pages: 1
});

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

function editCompany(item) {
  companyForm.id = Number(item.id || 0);
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

function openCompanyJobs(item) {
  router.push({ name: "job-center-jobs", query: { company_id: String(item.id) } });
}

async function loadCompanies(page = 1) {
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
    if (queryForm.domain_area.trim()) {
      params.domain_area = queryForm.domain_area.trim();
    }
    if (queryForm.province.trim()) {
      params.province = queryForm.province.trim();
    }
    if (queryForm.city.trim()) {
      params.city = queryForm.city.trim();
    }

    const resp = await api.get("/companies", { params });
    companies.value = resp?.data?.data || [];
    pager.page = Number(resp?.data?.page || params.page);
    pager.page_size = Number(resp?.data?.page_size || params.page_size);
    pager.total_count = Number(resp?.data?.count || 0);
    pager.total_pages = Number(resp?.data?.total_pages || 1);
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

function search() {
  loadCompanies(1);
}

function resetSearch() {
  queryForm.keyword = "";
  queryForm.domain_area = "";
  queryForm.province = "";
  queryForm.city = "";
  loadCompanies(1);
}

async function submitCompany() {
  if (!companyForm.name.trim()) {
    setError(new Error("企业名称不能为空"));
    return;
  }

  submitLoading.value = true;
  try {
    const payload = {
      name: companyForm.name.trim(),
      main_business: companyForm.main_business.trim() || "待补充",
      flagship_products: companyForm.flagship_products.trim() || "待补充",
      domain_area: companyForm.domain_area.trim() || "待补充",
      application_industries: companyForm.application_industries.trim() || "待补充",
      province: companyForm.province.trim(),
      city: companyForm.city.trim(),
      website: companyForm.website.trim(),
      description: companyForm.description.trim()
    };
    if (companyForm.id) {
      await api.put(`/companies/${companyForm.id}`, payload);
      setSuccess("企业修改成功");
    } else {
      await api.post("/companies", payload);
      setSuccess("企业新增成功");
    }
    resetCompanyForm();
    await loadCompanies(1);
  } catch (err) {
    setError(err);
  } finally {
    submitLoading.value = false;
  }
}

async function removeCompany(item) {
  if (!window.confirm(`确认删除企业「${item.name}」吗？`)) {
    return;
  }
  try {
    await api.delete(`/companies/${item.id}`);
    setSuccess("企业删除成功");
    await loadCompanies(pager.page);
  } catch (err) {
    setError(err);
  }
}

onMounted(() => {
  loadCompanies(1);
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
</style>
