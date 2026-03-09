<template>
  <section class="page">
    <div class="head-row">
      <button class="secondary" @click="goBack">返回专业列表</button>
      <button class="secondary" :disabled="loading" @click="initialize">刷新</button>
      <h2>专业详情中心</h2>
    </div>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel">
      <h3>{{ major.name || "-" }}（{{ major.code || "-" }}）</h3>
      <p class="hint">
        学院：{{ major.college_name || "-" }}
      </p>
      <p class="hint">{{ major.description || "暂无专业描述" }}</p>
    </div>

    <div class="tabs panel">
      <button :class="{ active: activeTab === 'profile' }" @click="activeTab = 'profile'">专业画像</button>
      <button :class="{ active: activeTab === 'students' }" @click="activeTab = 'students'">学生名单</button>
      <button :class="{ active: activeTab === 'employment' }" @click="activeTab = 'employment'">就业记录</button>
      <button :class="{ active: activeTab === 'match' }" @click="activeTab = 'match'">匹配结果</button>
    </div>

    <div v-if="activeTab === 'profile'" class="panel">
      <h3>专业画像文本</h3>
      <label>
        核心课程
        <textarea :value="profile.core_courses_text || ''" rows="4" readonly></textarea>
      </label>
      <label>
        专业知识与技能
        <textarea :value="profile.knowledge_skills_text || ''" rows="4" readonly></textarea>
      </label>
      <label>
        技术平台
        <textarea :value="profile.technical_platform_text || ''" rows="4" readonly></textarea>
      </label>
      <label>
        能力培养
        <textarea :value="profile.capability_text || ''" rows="4" readonly></textarea>
      </label>
    </div>

    <div v-if="activeTab === 'students'" class="panel">
      <details class="form-panel collapsible-panel">
        <summary class="panel-summary">{{ studentForm.id ? "编辑学生" : "新增学生" }}</summary>
        <div class="panel-body">
          <div class="grid">
            <label>
              学号
              <input v-model.trim="studentForm.student_no" placeholder="如 S2027001" />
            </label>
            <label>
              姓名
              <input v-model.trim="studentForm.full_name" placeholder="请输入姓名" />
            </label>
            <label>
              性别
              <select v-model="studentForm.gender">
                <option value="unknown">未知</option>
                <option value="male">男</option>
                <option value="female">女</option>
                <option value="other">其他</option>
              </select>
            </label>
            <label>
              毕业年份
              <input v-model.number="studentForm.graduation_year" type="number" min="2000" />
            </label>
            <label>
              邮箱
              <input v-model.trim="studentForm.email" placeholder="可选" />
            </label>
            <label>
              电话
              <input v-model.trim="studentForm.phone" placeholder="可选" />
            </label>
          </div>
          <label>
            简历文本
            <textarea v-model.trim="studentForm.resume_text" rows="2"></textarea>
          </label>
          <label>
            技术项目文本
            <textarea v-model.trim="studentForm.technical_projects_text" rows="2"></textarea>
          </label>
          <label>
            能力总结文本
            <textarea v-model.trim="studentForm.capability_summary_text" rows="2"></textarea>
          </label>
          <div class="actions">
            <button :disabled="loading" @click="submitStudent">{{ studentForm.id ? "保存修改" : "新增学生" }}</button>
            <button class="secondary" :disabled="loading" @click="resetStudentForm">清空</button>
          </div>
        </div>
      </details>

      <h3>学生名单（{{ students.length }}）</h3>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>学号</th>
            <th>姓名</th>
            <th>毕业年份</th>
            <th>联系方式</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in students" :key="item.id">
            <td>{{ index + 1 }}</td>
            <td>{{ item.student_no }}</td>
            <td>{{ item.full_name }}</td>
            <td>{{ item.graduation_year }}</td>
            <td>{{ item.phone || item.email || "-" }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="selectStudent(item)">详情</button>
              <button class="small secondary" @click="editStudent(item)">编辑</button>
              <button class="small danger" @click="removeStudent(item)">删除</button>
            </td>
          </tr>
          <tr v-if="!students.length">
            <td colspan="6">暂无学生数据</td>
          </tr>
        </tbody>
      </table>

      <div v-if="selectedStudent" class="panel student-detail">
        <div class="detail-head">
          <h4>学生详情：{{ selectedStudent.full_name }}（{{ selectedStudent.student_no }}）</h4>
          <button class="secondary small" @click="selectedStudent = null">关闭</button>
        </div>
        <p class="hint">
          基础信息：性别 {{ selectedStudent.gender || "-" }}，毕业年份 {{ selectedStudent.graduation_year || "-" }}，
          电话 {{ selectedStudent.phone || "-" }}，邮箱 {{ selectedStudent.email || "-" }}
        </p>
        <p class="hint">
          专业信息（只读）：{{ major.college_name || "-" }} / {{ major.name || "-" }}
        </p>
        <p class="hint">简历文本：{{ selectedStudent.resume_text || "暂无" }}</p>
        <p class="hint">项目文本：{{ selectedStudent.technical_projects_text || "暂无" }}</p>
        <p class="hint">能力总结：{{ selectedStudent.capability_summary_text || "暂无" }}</p>

        <h4>该学生就业记录（{{ selectedStudentEmployments.length }}）</h4>
        <table>
          <thead>
            <tr>
              <th>序号</th>
              <th>企业</th>
              <th>岗位</th>
              <th>类型</th>
              <th>状态</th>
              <th>开始日期</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(item, index) in selectedStudentEmployments" :key="item.id">
              <td>{{ index + 1 }}</td>
              <td>{{ item.company_name }}</td>
              <td>{{ item.job_title || "-" }}</td>
              <td>{{ item.employment_type }}</td>
              <td>{{ item.employment_status }}</td>
              <td>{{ item.start_date || "-" }}</td>
              <td>
                <button
                  class="small"
                  :disabled="!item.job_id || loading"
                  @click="runSingleMatch(Number(item.student_id || 0), Number(item.job_id || 0), true)"
                >
                  匹配
                </button>
              </td>
            </tr>
            <tr v-if="!selectedStudentEmployments.length">
              <td colspan="7">该学生暂无就业记录</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div v-if="activeTab === 'employment'" class="panel">
      <details class="form-panel collapsible-panel">
        <summary class="panel-summary">{{ employmentForm.id ? "编辑就业记录" : "新增就业记录" }}</summary>
        <div class="panel-body">
          <div class="grid">
            <label>
              学生
              <select v-model.number="employmentForm.student_id">
                <option :value="0">请选择学生</option>
                <option v-for="item in students" :key="item.id" :value="item.id">
                  {{ item.full_name }}（{{ item.student_no }}）
                </option>
              </select>
            </label>
            <label>
              企业
              <select v-model.number="employmentForm.company_id">
                <option :value="0">请选择企业</option>
                <option v-for="item in companies" :key="item.id" :value="item.id">
                  {{ item.name }}
                </option>
              </select>
            </label>
            <label>
              岗位（可选）
              <select v-model.number="employmentForm.job_id">
                <option :value="0">不关联岗位</option>
                <option v-for="item in visibleJobs" :key="item.id" :value="item.id">
                  {{ item.title }}（{{ item.company_name || "企业" }}）
                </option>
              </select>
            </label>
            <label>
              就业类型
              <select v-model="employmentForm.employment_type">
                <option value="signed">签约</option>
                <option value="offer">offer</option>
                <option value="internship">实习</option>
                <option value="pending">待定</option>
              </select>
            </label>
            <label>
              就业状态
              <select v-model="employmentForm.employment_status">
                <option value="active">在职</option>
                <option value="left">离职</option>
                <option value="unknown">未知</option>
              </select>
            </label>
            <label>
              开始日期
              <input v-model="employmentForm.start_date" type="date" />
            </label>
            <label>
              结束日期
              <input v-model="employmentForm.end_date" type="date" />
            </label>
            <label>
              证据来源
              <input v-model.trim="employmentForm.evidence_source" placeholder="可选" />
            </label>
          </div>
          <label>
            备注
            <textarea v-model.trim="employmentForm.notes" rows="2"></textarea>
          </label>
          <div class="actions">
            <button :disabled="loading" @click="submitEmployment(false)">{{ employmentForm.id ? "保存修改" : "新增记录" }}</button>
            <button class="secondary" :disabled="loading" @click="submitEmployment(true)">
              {{ employmentForm.id ? "保存并匹配" : "新增并匹配" }}
            </button>
            <button class="secondary" :disabled="loading" @click="resetEmploymentForm">清空</button>
          </div>
        </div>
      </details>

      <h3>就业记录（{{ employments.length }}）</h3>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>学生</th>
            <th>企业</th>
            <th>岗位</th>
            <th>类型</th>
            <th>状态</th>
            <th>开始日期</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in employments" :key="item.id">
            <td>{{ index + 1 }}</td>
            <td>{{ item.student_name }}</td>
            <td>{{ item.company_name }}</td>
            <td>{{ item.job_title || "-" }}</td>
            <td>{{ item.employment_type }}</td>
            <td>{{ item.employment_status }}</td>
            <td>{{ item.start_date || "-" }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="editEmployment(item)">编辑</button>
              <button class="small" :disabled="!item.job_id || loading" @click="runSingleMatch(Number(item.student_id || 0), Number(item.job_id || 0), true)">
                匹配
              </button>
              <button class="small danger" @click="removeEmployment(item)">删除</button>
            </td>
          </tr>
          <tr v-if="!employments.length">
            <td colspan="8">暂无就业记录</td>
          </tr>
        </tbody>
      </table>
    </div>

    <div v-if="activeTab === 'match'" class="panel">
      <h3>匹配结果（{{ matchResults.length }}）</h3>
      <details class="form-panel collapsible-panel">
        <summary class="panel-summary">匹配操作</summary>
        <div class="panel-body">
          <div class="grid">
            <label>
              匹配算法版本
              <input v-model.trim="matchControl.algorithm_version" placeholder="如 v0.4-major-student" />
            </label>
          </div>
          <div class="actions">
            <button :disabled="loading" @click="recomputeMajorMatches">按本专业批量重算并写入看板</button>
            <button class="secondary" :disabled="loading" @click="loadMatchResults">刷新结果</button>
          </div>
          <p v-if="recomputeSummary" class="hint">
            最近批量重算：总 {{ recomputeSummary.total_pairs }}，成功 {{ recomputeSummary.success_count }}，失败 {{ recomputeSummary.failed_count }}。
          </p>
        </div>
      </details>

      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>学生</th>
            <th>岗位</th>
            <th>算法版本</th>
            <th>总分</th>
            <th>技能分</th>
            <th>语义分</th>
            <th>约束系数</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in matchResults" :key="item.id">
            <td>{{ index + 1 }}</td>
            <td>{{ item.student_name || item.student_id }}</td>
            <td>{{ item.job_title || item.job_id }}</td>
            <td>{{ item.algorithm_version }}</td>
            <td>{{ item.final_score }}</td>
            <td>{{ item.skill_score }}</td>
            <td>{{ item.semantic_score }}</td>
            <td>{{ item.constraint_score }}</td>
          </tr>
          <tr v-if="!matchResults.length">
            <td colspan="8">暂无匹配结果</td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import { useRoute, useRouter } from "vue-router";
import api from "../services/api";

const route = useRoute();
const router = useRouter();
const majorId = Number(route.params.id || 0);

const activeTab = ref("profile");
const errorText = ref("");
const successText = ref("");
const loading = ref(false);

const major = ref({});
const profile = ref({});
const students = ref([]);
const employments = ref([]);
const matchResults = ref([]);
const companies = ref([]);
const jobs = ref([]);
const selectedStudent = ref(null);
const recomputeSummary = ref(null);

const studentForm = reactive({
  id: 0,
  student_no: "",
  full_name: "",
  gender: "unknown",
  graduation_year: new Date().getFullYear(),
  email: "",
  phone: "",
  resume_text: "",
  technical_projects_text: "",
  capability_summary_text: ""
});

const employmentForm = reactive({
  id: 0,
  student_id: 0,
  company_id: 0,
  job_id: 0,
  employment_type: "signed",
  employment_status: "active",
  start_date: "",
  end_date: "",
  evidence_source: "",
  notes: ""
});

const matchControl = reactive({
  algorithm_version: "v0.4-major-student"
});

const selectedStudentEmployments = computed(() => {
  if (!selectedStudent.value?.id) {
    return [];
  }
  const studentId = Number(selectedStudent.value.id);
  return employments.value.filter((item) => Number(item.student_id || 0) === studentId);
});

const visibleJobs = computed(() => {
  const companyId = Number(employmentForm.company_id || 0);
  if (companyId <= 0) {
    return jobs.value;
  }
  return jobs.value.filter((item) => Number(item.company_id || 0) === companyId);
});

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "操作失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

function goBack() {
  router.push("/majors");
}

function resetStudentForm() {
  studentForm.id = 0;
  studentForm.student_no = "";
  studentForm.full_name = "";
  studentForm.gender = "unknown";
  studentForm.graduation_year = new Date().getFullYear();
  studentForm.email = "";
  studentForm.phone = "";
  studentForm.resume_text = "";
  studentForm.technical_projects_text = "";
  studentForm.capability_summary_text = "";
}

function resetEmploymentForm() {
  employmentForm.id = 0;
  employmentForm.student_id = 0;
  employmentForm.company_id = 0;
  employmentForm.job_id = 0;
  employmentForm.employment_type = "signed";
  employmentForm.employment_status = "active";
  employmentForm.start_date = "";
  employmentForm.end_date = "";
  employmentForm.evidence_source = "";
  employmentForm.notes = "";
}

function selectStudent(student) {
  selectedStudent.value = student || null;
}

function editStudent(item) {
  studentForm.id = Number(item.id || 0);
  studentForm.student_no = item.student_no || "";
  studentForm.full_name = item.full_name || "";
  studentForm.gender = item.gender || "unknown";
  studentForm.graduation_year = Number(item.graduation_year || new Date().getFullYear());
  studentForm.email = item.email || "";
  studentForm.phone = item.phone || "";
  studentForm.resume_text = item.resume_text || "";
  studentForm.technical_projects_text = item.technical_projects_text || "";
  studentForm.capability_summary_text = item.capability_summary_text || "";
}

function editEmployment(item) {
  employmentForm.id = Number(item.id || 0);
  employmentForm.student_id = Number(item.student_id || 0);
  employmentForm.company_id = Number(item.company_id || 0);
  employmentForm.job_id = Number(item.job_id || 0);
  employmentForm.employment_type = item.employment_type || "signed";
  employmentForm.employment_status = item.employment_status || "active";
  employmentForm.start_date = item.start_date || "";
  employmentForm.end_date = item.end_date || "";
  employmentForm.evidence_source = item.evidence_source || "";
  employmentForm.notes = item.notes || "";
}

async function loadMajor() {
  const resp = await api.get(`/majors/${majorId}`);
  major.value = resp?.data?.data || {};
}

async function loadProfile() {
  try {
    const resp = await api.get(`/major-profiles/${majorId}`);
    profile.value = resp?.data?.data || {};
  } catch (err) {
    if (err?.response?.status === 404) {
      profile.value = {};
      return;
    }
    throw err;
  }
}

async function loadCompaniesAndJobs() {
  const [companyResp, jobResp] = await Promise.all([
    api.get("/companies", { params: { page: 1, page_size: 500, sort_by: "id", sort_order: "desc" } }),
    api.get("/jobs", { params: { page: 1, page_size: 1000, sort_by: "id", sort_order: "desc" } })
  ]);
  companies.value = companyResp?.data?.data || [];
  jobs.value = jobResp?.data?.data || [];
}

async function loadStudents() {
  const resp = await api.get("/students", {
    params: { page: 1, page_size: 1000, major_id: majorId, sort_by: "id", sort_order: "desc" }
  });
  students.value = resp?.data?.data || [];
}

async function loadEmployments() {
  const resp = await api.get("/employment-records", {
    params: { page: 1, page_size: 1000, major_id: majorId, sort_by: "id", sort_order: "desc" }
  });
  employments.value = resp?.data?.data || [];
}

async function loadMatchResults() {
  const resp = await api.get("/match-results", {
    params: { page: 1, page_size: 1000, major_id: majorId, sort_by: "id", sort_order: "desc" }
  });
  matchResults.value = resp?.data?.data || [];
}

async function submitStudent() {
  if (!major.value?.college_id || !majorId) {
    setError(new Error("当前专业信息不完整，无法提交学生"));
    return;
  }
  loading.value = true;
  try {
    const payload = {
      student_no: studentForm.student_no,
      full_name: studentForm.full_name,
      gender: studentForm.gender,
      college_id: Number(major.value.college_id),
      major_id: Number(majorId),
      graduation_year: Number(studentForm.graduation_year),
      email: studentForm.email,
      phone: studentForm.phone,
      resume_text: studentForm.resume_text,
      technical_projects_text: studentForm.technical_projects_text,
      capability_summary_text: studentForm.capability_summary_text
    };

    if (studentForm.id > 0) {
      await api.put(`/students/${studentForm.id}`, payload);
      setSuccess("学生信息修改成功");
    } else {
      await api.post("/students", payload);
      setSuccess("学生新增成功");
    }
    resetStudentForm();
    await loadStudents();
    await loadEmployments();
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

async function removeStudent(item) {
  if (!window.confirm(`确认删除学生「${item.full_name}」吗？`)) {
    return;
  }
  loading.value = true;
  try {
    await api.delete(`/students/${item.id}`);
    setSuccess("学生删除成功");
    if (selectedStudent.value?.id === item.id) {
      selectedStudent.value = null;
    }
    await loadStudents();
    await loadEmployments();
    await loadMatchResults();
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

async function runSingleMatch(studentId, jobId, showSuccess = false) {
  if (studentId <= 0 || jobId <= 0) {
    setError(new Error("缺少有效 student_id/job_id，无法匹配"));
    return;
  }
  loading.value = true;
  try {
    const resp = await api.post("/match/single", {
      student_id: studentId,
      job_id: jobId,
      algorithm_version: matchControl.algorithm_version || "v0.4-major-student",
      persist: true
    });
    if (showSuccess) {
      const score = Number(resp?.data?.data?.final_score || 0).toFixed(3);
      setSuccess(`匹配完成，学生 ${studentId} 最终分 ${score}`);
    }
    await loadMatchResults();
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

async function submitEmployment(runMatchAfterSave) {
  loading.value = true;
  try {
    const payload = {
      student_id: Number(employmentForm.student_id || 0),
      company_id: Number(employmentForm.company_id || 0),
      job_id: Number(employmentForm.job_id || 0),
      employment_type: employmentForm.employment_type,
      employment_status: employmentForm.employment_status,
      start_date: employmentForm.start_date,
      end_date: employmentForm.end_date,
      evidence_source: employmentForm.evidence_source,
      notes: employmentForm.notes
    };

    if (employmentForm.id > 0) {
      await api.put(`/employment-records/${employmentForm.id}`, payload);
      setSuccess("就业记录修改成功");
    } else {
      await api.post("/employment-records", payload);
      setSuccess("就业记录新增成功");
    }

    const studentId = payload.student_id;
    const jobId = payload.job_id;

    resetEmploymentForm();
    await loadEmployments();

    if (runMatchAfterSave && studentId > 0 && jobId > 0) {
      await runSingleMatch(studentId, jobId, true);
    }
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

async function removeEmployment(item) {
  if (!window.confirm(`确认删除就业记录 #${item.id} 吗？`)) {
    return;
  }
  loading.value = true;
  try {
    await api.delete(`/employment-records/${item.id}`);
    setSuccess("就业记录删除成功");
    await loadEmployments();
    await loadMatchResults();
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

async function recomputeMajorMatches() {
  loading.value = true;
  try {
    const resp = await api.post(`/match/major/${majorId}/recompute`, {
      algorithm_version: matchControl.algorithm_version || "v0.4-major-student",
      persist: true
    });
    recomputeSummary.value = resp?.data?.summary || null;
    setSuccess(
      `批量重算完成：成功 ${recomputeSummary.value?.success_count || 0}，失败 ${recomputeSummary.value?.failed_count || 0}`
    );
    await loadMatchResults();
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
  }
}

async function initialize() {
  if (!majorId) {
    setError(new Error("无效的专业ID"));
    return;
  }
  loading.value = true;
  try {
    await loadMajor();
    await loadProfile();
    await loadCompaniesAndJobs();
    await loadStudents();
    await loadEmployments();
    await loadMatchResults();
  } catch (err) {
    setError(err);
  } finally {
    loading.value = false;
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

.head-row {
  display: flex;
  align-items: center;
  gap: 12px;
}

.panel {
  margin-top: 14px;
  border: 1px solid #dfe7ef;
  border-radius: 8px;
  padding: 12px 14px;
  background: #fafcff;
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

.hint {
  margin: 4px 0;
  color: #4c5a67;
  font-size: 13px;
}

.tabs {
  display: flex;
  gap: 8px;
}

.tabs button {
  border: 1px solid #5f86aa;
  background: #fff;
  color: #0d2b45;
  border-radius: 6px;
  padding: 7px 12px;
  cursor: pointer;
}

.tabs button.active {
  border-color: #0d2b45;
  background: #0d2b45;
  color: #fff;
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
  margin-top: 8px;
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
  margin-top: 8px;
}

th,
td {
  border-bottom: 1px solid #e6edf4;
  padding: 8px 6px;
  text-align: left;
}

.actions {
  display: flex;
  gap: 8px;
  margin-top: 8px;
}

.row-actions {
  white-space: nowrap;
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
  border: 1px solid #5f86aa;
  background: #fff;
  color: #0d2b45;
}

.danger {
  border-color: #b42318;
  background: #fff;
  color: #b42318;
}

.error {
  margin: 8px 0;
  color: #b42318;
}

.success {
  margin: 8px 0;
  color: #027a48;
}

.small {
  padding: 4px 8px;
  font-size: 12px;
}

.student-detail {
  margin-top: 10px;
  background: #fff;
}

.detail-head {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 8px;
}
</style>
