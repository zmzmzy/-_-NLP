<template>
  <section class="page">
    <h2>专业信息管理</h2>
    <p class="desc">
      本页已接入学院、专业、专业画像接口，可进行新增、编辑、删除与画像维护。
    </p>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel form-panel">
      <h3>{{ collegeForm.id ? "编辑学院" : "新增学院" }}</h3>
      <div class="grid">
        <label>
          学院编码
          <input v-model.trim="collegeForm.code" placeholder="如 C01" />
        </label>
        <label>
          学院名称
          <input v-model.trim="collegeForm.name" placeholder="请输入学院名称" />
        </label>
      </div>
      <label>
        学院描述
        <textarea v-model.trim="collegeForm.description" rows="2" placeholder="可选"></textarea>
      </label>
      <div class="actions">
        <button @click="submitCollege">{{ collegeForm.id ? "保存修改" : "新增学院" }}</button>
        <button class="secondary" @click="resetCollegeForm">清空</button>
      </div>
    </div>

    <div class="panel">
      <h3>学院列表</h3>
      <table>
        <thead>
          <tr>
            <th>ID</th>
            <th>编码</th>
            <th>名称</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in colleges" :key="item.id">
            <td>{{ item.id }}</td>
            <td>{{ item.code }}</td>
            <td>{{ item.name }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="editCollege(item)">编辑</button>
              <button class="small danger" @click="removeCollege(item)">删除</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="panel form-panel">
      <h3>{{ majorForm.id ? "编辑专业" : "新增专业" }}</h3>
      <div class="grid">
        <label>
          所属学院
          <select v-model.number="majorForm.college_id">
            <option :value="0">请选择学院</option>
            <option v-for="c in colleges" :key="c.id" :value="c.id">{{ c.name }}</option>
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
        <label>
          学历层次
          <select v-model="majorForm.education_level">
            <option value="bachelor">本科</option>
            <option value="master">硕士</option>
            <option value="phd">博士</option>
          </select>
        </label>
        <label>
          学科门类
          <input v-model.trim="majorForm.discipline_category" placeholder="如 Engineering" />
        </label>
        <label>
          入学年份
          <input v-model.number="majorForm.enrollment_year" type="number" min="0" placeholder="可选" />
        </label>
      </div>
      <label>
        专业描述
        <textarea v-model.trim="majorForm.description" rows="2" placeholder="可选"></textarea>
      </label>
      <div class="actions">
        <button @click="submitMajor">{{ majorForm.id ? "保存修改" : "新增专业" }}</button>
        <button class="secondary" @click="resetMajorForm">清空</button>
      </div>
    </div>

    <div class="panel">
      <h3>专业列表</h3>
      <table>
        <thead>
          <tr>
            <th>ID</th>
            <th>专业编码</th>
            <th>专业名称</th>
            <th>学院</th>
            <th>学历层次</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in majors" :key="item.id">
            <td>{{ item.id }}</td>
            <td>{{ item.code }}</td>
            <td>{{ item.name }}</td>
            <td>{{ item.college_name }}</td>
            <td>{{ item.education_level }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="editMajor(item)">编辑</button>
              <button class="small secondary" @click="selectMajorForProfile(item)">画像</button>
              <button class="small danger" @click="removeMajor(item)">删除</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="panel form-panel">
      <h3>专业画像维护</h3>
      <label>
        选择专业
        <select v-model.number="profileMajorId" @change="loadMajorProfile">
          <option :value="0">请选择专业</option>
          <option v-for="m in majors" :key="m.id" :value="m.id">{{ m.name }}</option>
        </select>
      </label>
      <label>
        核心课程描述
        <textarea v-model.trim="profileForm.core_courses_text" rows="3"></textarea>
      </label>
      <label>
        专业知识与技能描述
        <textarea v-model.trim="profileForm.knowledge_skills_text" rows="3"></textarea>
      </label>
      <label>
        技术平台描述
        <textarea v-model.trim="profileForm.technical_platform_text" rows="3"></textarea>
      </label>
      <label>
        能力培养描述
        <textarea v-model.trim="profileForm.capability_text" rows="3"></textarea>
      </label>
      <div class="actions">
        <button @click="submitMajorProfile">保存专业画像</button>
      </div>
    </div>
  </section>
</template>

<script setup>
import { onMounted, reactive, ref } from "vue";
import api from "../services/api";

const colleges = ref([]);
const majors = ref([]);
const profileMajorId = ref(0);

const errorText = ref("");
const successText = ref("");

const collegeForm = reactive({
  id: 0,
  code: "",
  name: "",
  description: ""
});

const majorForm = reactive({
  id: 0,
  college_id: 0,
  code: "",
  name: "",
  education_level: "bachelor",
  discipline_category: "",
  enrollment_year: 0,
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

function resetCollegeForm() {
  collegeForm.id = 0;
  collegeForm.code = "";
  collegeForm.name = "";
  collegeForm.description = "";
}

function resetMajorForm() {
  majorForm.id = 0;
  majorForm.college_id = 0;
  majorForm.code = "";
  majorForm.name = "";
  majorForm.education_level = "bachelor";
  majorForm.discipline_category = "";
  majorForm.enrollment_year = 0;
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

async function loadMajors() {
  const resp = await api.get("/majors", {
    params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" }
  });
  majors.value = resp?.data?.data || [];
}

function editCollege(item) {
  collegeForm.id = item.id;
  collegeForm.code = item.code || "";
  collegeForm.name = item.name || "";
  collegeForm.description = item.description || "";
}

async function submitCollege() {
  try {
    const payload = {
      code: collegeForm.code,
      name: collegeForm.name,
      description: collegeForm.description
    };
    if (collegeForm.id) {
      await api.put(`/colleges/${collegeForm.id}`, payload);
      setSuccess("学院修改成功");
    } else {
      await api.post("/colleges", payload);
      setSuccess("学院新增成功");
    }
    resetCollegeForm();
    await loadColleges();
  } catch (err) {
    setError(err);
  }
}

async function removeCollege(item) {
  if (!window.confirm(`确认删除学院「${item.name}」吗？`)) {
    return;
  }
  try {
    await api.delete(`/colleges/${item.id}`);
    setSuccess("学院删除成功");
    await loadColleges();
  } catch (err) {
    setError(err);
  }
}

function editMajor(item) {
  majorForm.id = item.id;
  majorForm.college_id = Number(item.college_id || 0);
  majorForm.code = item.code || "";
  majorForm.name = item.name || "";
  majorForm.education_level = item.education_level || "bachelor";
  majorForm.discipline_category = item.discipline_category || "";
  majorForm.enrollment_year = Number(item.enrollment_year || 0);
  majorForm.description = item.description || "";
}

async function submitMajor() {
  try {
    const payload = {
      college_id: Number(majorForm.college_id),
      code: majorForm.code,
      name: majorForm.name,
      education_level: majorForm.education_level,
      discipline_category: majorForm.discipline_category,
      enrollment_year: Number(majorForm.enrollment_year || 0),
      description: majorForm.description
    };
    if (majorForm.id) {
      await api.put(`/majors/${majorForm.id}`, payload);
      setSuccess("专业修改成功");
    } else {
      await api.post("/majors", payload);
      setSuccess("专业新增成功");
    }
    resetMajorForm();
    await loadMajors();
  } catch (err) {
    setError(err);
  }
}

async function removeMajor(item) {
  if (!window.confirm(`确认删除专业「${item.name}」吗？`)) {
    return;
  }
  try {
    await api.delete(`/majors/${item.id}`);
    setSuccess("专业删除成功");
    await loadMajors();
  } catch (err) {
    setError(err);
  }
}

function selectMajorForProfile(item) {
  profileMajorId.value = Number(item.id);
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
    if (err?.response?.status === 404) {
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
  try {
    const payload = {
      core_courses_text: profileForm.core_courses_text,
      knowledge_skills_text: profileForm.knowledge_skills_text,
      technical_platform_text: profileForm.technical_platform_text,
      capability_text: profileForm.capability_text
    };
    await api.put(`/major-profiles/${profileMajorId.value}`, payload);
    setSuccess("专业画像保存成功");
    await loadMajorProfile();
  } catch (err) {
    setError(err);
  }
}

async function initialize() {
  try {
    await loadColleges();
    await loadMajors();
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
