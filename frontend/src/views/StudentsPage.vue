<template>
  <section class="page">
    <h2>学生与就业记录管理</h2>
    <p class="desc">
      本页已接入学生与就业记录接口，可维护学生基本信息、简历文本以及就业去向记录。
    </p>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel form-panel">
      <h3>{{ studentForm.id ? "编辑学生" : "新增学生" }}</h3>
      <div class="grid">
        <label>
          学号
          <input v-model.trim="studentForm.student_no" placeholder="如 20240001" />
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
          学院
          <select v-model.number="studentForm.college_id">
            <option :value="0">请选择学院</option>
            <option v-for="c in colleges" :key="c.id" :value="c.id">{{ c.name }}</option>
          </select>
        </label>
        <label>
          专业
          <select v-model.number="studentForm.major_id">
            <option :value="0">请选择专业</option>
            <option v-for="m in majors" :key="m.id" :value="m.id">{{ m.name }}</option>
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
        <button @click="submitStudent">{{ studentForm.id ? "保存修改" : "新增学生" }}</button>
        <button class="secondary" @click="resetStudentForm">清空</button>
      </div>
    </div>

    <div class="panel">
      <h3>学生列表</h3>
      <table>
        <thead>
          <tr>
            <th>ID</th>
            <th>学号</th>
            <th>姓名</th>
            <th>专业</th>
            <th>毕业年份</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in students" :key="item.id">
            <td>{{ item.id }}</td>
            <td>{{ item.student_no }}</td>
            <td>{{ item.full_name }}</td>
            <td>{{ item.major_name }}</td>
            <td>{{ item.graduation_year }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="editStudent(item)">编辑</button>
              <button class="small danger" @click="removeStudent(item)">删除</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="panel form-panel">
      <h3>{{ employmentForm.id ? "编辑就业记录" : "新增就业记录" }}</h3>
      <div class="grid">
        <label>
          学生
          <select v-model.number="employmentForm.student_id">
            <option :value="0">请选择学生</option>
            <option v-for="s in students" :key="s.id" :value="s.id">{{ s.full_name }}（{{ s.student_no }}）</option>
          </select>
        </label>
        <label>
          企业
          <select v-model.number="employmentForm.company_id">
            <option :value="0">请选择企业</option>
            <option v-for="c in companies" :key="c.id" :value="c.id">{{ c.name }}</option>
          </select>
        </label>
        <label>
          岗位（可选）
          <select v-model.number="employmentForm.job_id">
            <option :value="0">不关联岗位</option>
            <option v-for="j in jobs" :key="j.id" :value="j.id">{{ j.title }}</option>
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
        <button @click="submitEmployment">{{ employmentForm.id ? "保存修改" : "新增记录" }}</button>
        <button class="secondary" @click="resetEmploymentForm">清空</button>
      </div>
    </div>

    <div class="panel">
      <h3>就业记录列表</h3>
      <table>
        <thead>
          <tr>
            <th>ID</th>
            <th>学生</th>
            <th>企业</th>
            <th>岗位</th>
            <th>类型</th>
            <th>状态</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in employments" :key="item.id">
            <td>{{ item.id }}</td>
            <td>{{ item.student_name }}</td>
            <td>{{ item.company_name }}</td>
            <td>{{ item.job_title }}</td>
            <td>{{ item.employment_type }}</td>
            <td>{{ item.employment_status }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="editEmployment(item)">编辑</button>
              <button class="small danger" @click="removeEmployment(item)">删除</button>
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

const colleges = ref([]);
const majors = ref([]);
const companies = ref([]);
const jobs = ref([]);
const students = ref([]);
const employments = ref([]);
const errorText = ref("");
const successText = ref("");

const studentForm = reactive({
  id: 0,
  student_no: "",
  full_name: "",
  gender: "unknown",
  college_id: 0,
  major_id: 0,
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

function setError(err) {
  errorText.value = err?.response?.data?.error || err?.message || "操作失败";
  successText.value = "";
}

function setSuccess(text) {
  successText.value = text;
  errorText.value = "";
}

function resetStudentForm() {
  studentForm.id = 0;
  studentForm.student_no = "";
  studentForm.full_name = "";
  studentForm.gender = "unknown";
  studentForm.college_id = 0;
  studentForm.major_id = 0;
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

async function loadBaseOptions() {
  const [collegeResp, majorResp, companyResp, jobResp] = await Promise.all([
    api.get("/colleges", { params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" } }),
    api.get("/majors", { params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" } }),
    api.get("/companies", { params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" } }),
    api.get("/jobs", { params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" } })
  ]);
  colleges.value = collegeResp?.data?.data || [];
  majors.value = majorResp?.data?.data || [];
  companies.value = companyResp?.data?.data || [];
  jobs.value = jobResp?.data?.data || [];
}

async function loadStudents() {
  const resp = await api.get("/students", {
    params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" }
  });
  students.value = resp?.data?.data || [];
}

async function loadEmployments() {
  const resp = await api.get("/employment-records", {
    params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" }
  });
  employments.value = resp?.data?.data || [];
}

function editStudent(item) {
  studentForm.id = item.id;
  studentForm.student_no = item.student_no || "";
  studentForm.full_name = item.full_name || "";
  studentForm.gender = item.gender || "unknown";
  studentForm.college_id = Number(item.college_id || 0);
  studentForm.major_id = Number(item.major_id || 0);
  studentForm.graduation_year = Number(item.graduation_year || new Date().getFullYear());
  studentForm.email = item.email || "";
  studentForm.phone = item.phone || "";
  studentForm.resume_text = item.resume_text || "";
  studentForm.technical_projects_text = item.technical_projects_text || "";
  studentForm.capability_summary_text = item.capability_summary_text || "";
}

async function submitStudent() {
  try {
    const payload = {
      student_no: studentForm.student_no,
      full_name: studentForm.full_name,
      gender: studentForm.gender,
      college_id: Number(studentForm.college_id),
      major_id: Number(studentForm.major_id),
      graduation_year: Number(studentForm.graduation_year),
      email: studentForm.email,
      phone: studentForm.phone,
      resume_text: studentForm.resume_text,
      technical_projects_text: studentForm.technical_projects_text,
      capability_summary_text: studentForm.capability_summary_text
    };
    if (studentForm.id) {
      await api.put(`/students/${studentForm.id}`, payload);
      setSuccess("学生信息修改成功");
    } else {
      await api.post("/students", payload);
      setSuccess("学生信息新增成功");
    }
    resetStudentForm();
    await loadStudents();
  } catch (err) {
    setError(err);
  }
}

async function removeStudent(item) {
  if (!window.confirm(`确认删除学生「${item.full_name}」吗？`)) {
    return;
  }
  try {
    await api.delete(`/students/${item.id}`);
    setSuccess("学生删除成功");
    await loadStudents();
    await loadEmployments();
  } catch (err) {
    setError(err);
  }
}

function editEmployment(item) {
  employmentForm.id = item.id;
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

async function submitEmployment() {
  try {
    const payload = {
      student_id: Number(employmentForm.student_id),
      company_id: Number(employmentForm.company_id),
      job_id: employmentForm.job_id ? Number(employmentForm.job_id) : null,
      employment_type: employmentForm.employment_type,
      employment_status: employmentForm.employment_status,
      start_date: employmentForm.start_date,
      end_date: employmentForm.end_date,
      evidence_source: employmentForm.evidence_source,
      notes: employmentForm.notes
    };
    if (employmentForm.id) {
      await api.put(`/employment-records/${employmentForm.id}`, payload);
      setSuccess("就业记录修改成功");
    } else {
      await api.post("/employment-records", payload);
      setSuccess("就业记录新增成功");
    }
    resetEmploymentForm();
    await loadEmployments();
  } catch (err) {
    setError(err);
  }
}

async function removeEmployment(item) {
  if (!window.confirm(`确认删除就业记录 #${item.id} 吗？`)) {
    return;
  }
  try {
    await api.delete(`/employment-records/${item.id}`);
    setSuccess("就业记录删除成功");
    await loadEmployments();
  } catch (err) {
    setError(err);
  }
}

async function initialize() {
  try {
    await loadBaseOptions();
    await loadStudents();
    await loadEmployments();
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
