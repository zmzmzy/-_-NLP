<template>
  <section class="mc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel filter-panel">
      <div class="grid">
        <label>
          关键词
          <input v-model.trim="queryForm.keyword" placeholder="专业编码 / 名称 / 学院 / 描述" @keyup.enter="search" />
        </label>
        <label>
          学院
          <select v-model.number="queryForm.college_id">
            <option :value="0">全部学院</option>
            <option v-for="c in colleges" :key="c.id" :value="c.id">{{ c.name }}</option>
          </select>
        </label>
      </div>
      <div class="actions">
        <button class="secondary" :disabled="loading" @click="search">查询</button>
        <button class="secondary" :disabled="loading" @click="resetSearch">重置</button>
        <router-link class="mc-link-btn secondary" :to="{ name: 'major-center-imports' }">去导入中心</router-link>
      </div>
    </div>

    <div class="panel">
      <div class="section-head">
        <h3>专业列表</h3>
        <p class="meta">共 {{ pager.total_count }} 条，当前第 {{ pager.page }} / {{ pager.total_pages }} 页</p>
      </div>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>专业编码</th>
            <th>专业名称</th>
            <th>所属学院</th>
            <th>描述</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in majors" :key="item.id">
            <td>{{ rowNo(index) }}</td>
            <td>{{ item.code }}</td>
            <td>{{ item.name }}</td>
            <td>{{ item.college_name }}</td>
            <td>{{ item.description || '-' }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="goMajorDetail(item)">详情</button>
              <button class="small secondary" @click="editMajor(item)">编辑</button>
              <button class="small secondary" @click="selectMajorForProfile(item)">画像</button>
              <button class="small danger" @click="removeMajor(item)">删除</button>
            </td>
          </tr>
          <tr v-if="!majors.length">
            <td colspan="6">暂无专业数据</td>
          </tr>
        </tbody>
      </table>
      <div class="actions page-actions">
        <button class="secondary" :disabled="loading || pager.page <= 1" @click="loadMajors(pager.page - 1)">上一页</button>
        <button class="secondary" :disabled="loading || pager.page >= pager.total_pages" @click="loadMajors(pager.page + 1)">下一页</button>
      </div>
    </div>

    <div class="split-grid">
      <div class="panel form-panel">
        <h3>{{ majorForm.id ? "编辑专业" : "新增专业" }}</h3>
        <div class="grid">
          <label>
            所属学院
            <select v-model.number="majorForm.college_id">
              <option :value="0">请选择学院</option>
              <option v-for="c in colleges" :key="`form-college-${c.id}`" :value="c.id">{{ c.name }}</option>
            </select>
          </label>
          <label>
            专业编码
            <input v-model.trim="majorForm.code" placeholder="如 M001" />
          </label>
          <label>
            专业名称
            <input v-model.trim="majorForm.name" placeholder="请输入专业名称" />
          </label>
        </div>
        <label>
          专业描述
          <textarea v-model.trim="majorForm.description" rows="3" placeholder="可选"></textarea>
        </label>
        <div class="actions">
          <button :disabled="submitLoading" @click="submitMajor">{{ submitLoading ? "提交中..." : (majorForm.id ? "保存修改" : "新增专业") }}</button>
          <button class="secondary" :disabled="submitLoading" @click="resetMajorForm">清空</button>
        </div>
      </div>

      <div class="panel form-panel">
        <h3>专业画像维护</h3>
        <label>
          选择专业
          <select v-model.number="profileMajorId" @change="loadMajorProfile">
            <option :value="0">请选择专业</option>
            <option v-for="m in majors" :key="`profile-major-${m.id}`" :value="m.id">{{ m.name }}</option>
          </select>
        </label>
        <label>
          核心课程描述
          <textarea v-model.trim="profileForm.core_courses_text" rows="2"></textarea>
        </label>
        <label>
          专业知识与技能描述
          <textarea v-model.trim="profileForm.knowledge_skills_text" rows="2"></textarea>
        </label>
        <label>
          技术平台描述
          <textarea v-model.trim="profileForm.technical_platform_text" rows="2"></textarea>
        </label>
        <label>
          能力培养描述
          <textarea v-model.trim="profileForm.capability_text" rows="2"></textarea>
        </label>
        <div class="actions">
          <button :disabled="profileSaving" @click="submitMajorProfile">{{ profileSaving ? "保存中..." : "保存专业画像" }}</button>
        </div>
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

const colleges = ref([]);
const majors = ref([]);
const loading = ref(false);
const submitLoading = ref(false);
const profileSaving = ref(false);
const profileMajorId = ref(0);
const errorText = ref("");
const successText = ref("");

const queryForm = reactive({
  keyword: "",
  college_id: 0
});

const pager = reactive({
  page: 1,
  page_size: 20,
  total_count: 0,
  total_pages: 1
});

const majorForm = reactive({
  id: 0,
  college_id: 0,
  code: "",
  name: "",
  description: ""
});

const profileForm = reactive({
  core_courses_text: "",
  knowledge_skills_text: "",
  technical_platform_text: "",
  capability_text: ""
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

function syncRouteCollegeFilter() {
  const collegeId = Number(queryForm.college_id || 0);
  const nextQuery = { ...route.query };
  if (collegeId > 0) {
    nextQuery.college_id = String(collegeId);
  } else {
    delete nextQuery.college_id;
  }
  router.replace({ query: nextQuery });
}

function resetMajorForm() {
  majorForm.id = 0;
  majorForm.college_id = 0;
  majorForm.code = "";
  majorForm.name = "";
  majorForm.description = "";
}

function resetProfileForm() {
  profileForm.core_courses_text = "";
  profileForm.knowledge_skills_text = "";
  profileForm.technical_platform_text = "";
  profileForm.capability_text = "";
}

async function loadColleges() {
  const resp = await api.get("/colleges", {
    params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" }
  });
  colleges.value = resp?.data?.data || [];
}

async function loadMajors(page = 1) {
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
    if (Number(queryForm.college_id || 0) > 0) {
      params.college_id = Number(queryForm.college_id);
    }
    const resp = await api.get("/majors", { params });
    majors.value = resp?.data?.data || [];
    pager.page = Number(resp?.data?.page || params.page);
    pager.page_size = Number(resp?.data?.page_size || params.page_size);
    pager.total_count = Number(resp?.data?.count || 0);
    pager.total_pages = Number(resp?.data?.total_pages || 1);
    syncRouteCollegeFilter();
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

function search() {
  loadMajors(1);
}

function resetSearch() {
  queryForm.keyword = "";
  queryForm.college_id = 0;
  loadMajors(1);
}

function editMajor(item) {
  majorForm.id = Number(item.id || 0);
  majorForm.college_id = Number(item.college_id || 0);
  majorForm.code = item.code || "";
  majorForm.name = item.name || "";
  majorForm.description = item.description || "";
}

function goMajorDetail(item) {
  router.push(`/majors/${item.id}`);
}

async function submitMajor() {
  if (!majorForm.college_id || !majorForm.code.trim() || !majorForm.name.trim()) {
    setError(new Error("所属学院、专业编码、专业名称不能为空"));
    return;
  }

  submitLoading.value = true;
  try {
    const payload = {
      college_id: Number(majorForm.college_id),
      code: majorForm.code.trim(),
      name: majorForm.name.trim(),
      description: majorForm.description.trim()
    };
    if (majorForm.id) {
      await api.put(`/majors/${majorForm.id}`, payload);
      setSuccess("专业修改成功");
    } else {
      await api.post("/majors", payload);
      setSuccess("专业新增成功");
    }
    resetMajorForm();
    await loadMajors(1);
  } catch (err) {
    setError(err);
  } finally {
    submitLoading.value = false;
  }
}

async function removeMajor(item) {
  if (!window.confirm(`确认删除专业「${item.name}」吗？`)) {
    return;
  }
  try {
    await api.delete(`/majors/${item.id}`);
    setSuccess("专业删除成功");
    await loadMajors(pager.page);
  } catch (err) {
    setError(err);
  }
}

function selectMajorForProfile(item) {
  profileMajorId.value = Number(item.id || 0);
  loadMajorProfile();
}

async function loadMajorProfile() {
  if (!profileMajorId.value) {
    resetProfileForm();
    return;
  }
  try {
    const resp = await api.get(`/major-profiles/${profileMajorId.value}`);
    const data = resp?.data?.data || {};
    profileForm.core_courses_text = data.core_courses_text || "";
    profileForm.knowledge_skills_text = data.knowledge_skills_text || "";
    profileForm.technical_platform_text = data.technical_platform_text || "";
    profileForm.capability_text = data.capability_text || "";
  } catch (err) {
    if (Number(err?.response?.status || 0) === 404) {
      resetProfileForm();
      return;
    }
    setError(err);
  }
}

async function submitMajorProfile() {
  if (!profileMajorId.value) {
    setError(new Error("请先选择专业"));
    return;
  }
  profileSaving.value = true;
  try {
    const payload = {
      core_courses_text: profileForm.core_courses_text,
      knowledge_skills_text: profileForm.knowledge_skills_text,
      technical_platform_text: profileForm.technical_platform_text,
      capability_text: profileForm.capability_text
    };
    await api.put(`/major-profiles/${profileMajorId.value}`, payload);
    setSuccess("专业画像保存成功");
  } catch (err) {
    setError(err);
  } finally {
    profileSaving.value = false;
  }
}

watch(
  () => route.query.college_id,
  (val) => {
    const q = Number(val || 0);
    if (q >= 0 && q !== Number(queryForm.college_id || 0)) {
      queryForm.college_id = q;
      loadMajors(1);
    }
  }
);

onMounted(async () => {
  try {
    await loadColleges();
    queryForm.college_id = Number(route.query.college_id || 0);
    await loadMajors(1);
  } catch (err) {
    setError(err);
  }
});
</script>

<style scoped>
.mc-view {
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

.split-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.mc-link-btn {
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

@media (max-width: 980px) {
  .split-grid {
    grid-template-columns: 1fr;
  }
}
</style>
