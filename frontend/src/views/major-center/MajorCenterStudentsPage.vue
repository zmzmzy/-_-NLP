<template>
  <section class="mc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel filter-panel">
      <div class="grid">
        <label>
          关键词
          <input v-model.trim="queryForm.keyword" placeholder="学号 / 姓名 / 邮箱 / 电话" @keyup.enter="search" />
        </label>
        <label>
          学院
          <select v-model.number="queryForm.college_id">
            <option :value="0">全部学院</option>
            <option v-for="c in colleges" :key="`q-college-${c.id}`" :value="c.id">{{ c.name }}</option>
          </select>
        </label>
        <label>
          专业
          <select v-model.number="queryForm.major_id">
            <option :value="0">全部专业</option>
            <option v-for="m in queryMajors" :key="`q-major-${m.id}`" :value="m.id">{{ m.name }}</option>
          </select>
        </label>
        <label>
          毕业年份
          <input v-model.number="queryForm.graduation_year" type="number" min="2000" />
        </label>
      </div>
      <div class="actions">
        <button class="secondary" :disabled="loading" @click="search">查询</button>
        <button class="secondary" :disabled="loading" @click="resetSearch">重置</button>
      </div>
    </div>

    <div class="panel">
      <div class="section-head">
        <h3>学生列表</h3>
        <p class="meta">共 {{ pager.total_count }} 条，当前第 {{ pager.page }} / {{ pager.total_pages }} 页</p>
      </div>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>学号</th>
            <th>姓名</th>
            <th>性别</th>
            <th>学院</th>
            <th>专业</th>
            <th>毕业年份</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in students" :key="item.id">
            <td>{{ rowNo(index) }}</td>
            <td>{{ item.student_no }}</td>
            <td>{{ item.full_name }}</td>
            <td>{{ item.gender }}</td>
            <td>{{ item.college_name }}</td>
            <td>{{ item.major_name }}</td>
            <td>{{ item.graduation_year }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="editStudent(item)">编辑</button>
              <button class="small danger" @click="removeStudent(item)">删除</button>
            </td>
          </tr>
          <tr v-if="!students.length">
            <td colspan="8">暂无学生数据</td>
          </tr>
        </tbody>
      </table>
      <div class="actions page-actions">
        <button class="secondary" :disabled="loading || pager.page <= 1" @click="loadStudents(pager.page - 1)">上一页</button>
        <button class="secondary" :disabled="loading || pager.page >= pager.total_pages" @click="loadStudents(pager.page + 1)">下一页</button>
      </div>
    </div>

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
            <option v-for="c in colleges" :key="`f-college-${c.id}`" :value="c.id">{{ c.name }}</option>
          </select>
        </label>
        <label>
          专业
          <select v-model.number="studentForm.major_id">
            <option :value="0">请选择专业</option>
            <option v-for="m in formMajors" :key="`f-major-${m.id}`" :value="m.id">{{ m.name }}</option>
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
        <button :disabled="submitLoading" @click="submitStudent">{{ submitLoading ? "提交中..." : (studentForm.id ? "保存修改" : "新增学生") }}</button>
        <button class="secondary" :disabled="submitLoading" @click="resetStudentForm">清空</button>
      </div>
    </div>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref, watch } from "vue";
import api from "../../services/api";

const colleges = ref([]);
const majors = ref([]);
const students = ref([]);

const loading = ref(false);
const submitLoading = ref(false);
const errorText = ref("");
const successText = ref("");

const queryForm = reactive({
  keyword: "",
  college_id: 0,
  major_id: 0,
  graduation_year: 0
});

const pager = reactive({
  page: 1,
  page_size: 20,
  total_count: 0,
  total_pages: 1
});

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

const queryMajors = computed(() => {
  if (!queryForm.college_id) {
    return majors.value;
  }
  return majors.value.filter((m) => Number(m.college_id) === Number(queryForm.college_id));
});

const formMajors = computed(() => {
  if (!studentForm.college_id) {
    return majors.value;
  }
  return majors.value.filter((m) => Number(m.college_id) === Number(studentForm.college_id));
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

async function loadBaseOptions() {
  const [collegeResp, majorResp] = await Promise.all([
    api.get("/colleges", { params: { page: 1, page_size: 200, sort_by: "id", sort_order: "desc" } }),
    api.get("/majors", { params: { page: 1, page_size: 300, sort_by: "id", sort_order: "desc" } })
  ]);
  colleges.value = collegeResp?.data?.data || [];
  majors.value = majorResp?.data?.data || [];
}

async function loadStudents(page = 1) {
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
    if (Number(queryForm.major_id || 0) > 0) {
      params.major_id = Number(queryForm.major_id);
    }
    if (Number(queryForm.graduation_year || 0) > 0) {
      params.graduation_year = Number(queryForm.graduation_year);
    }
    const resp = await api.get("/students", { params });
    students.value = resp?.data?.data || [];
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
  loadStudents(1);
}

function resetSearch() {
  queryForm.keyword = "";
  queryForm.college_id = 0;
  queryForm.major_id = 0;
  queryForm.graduation_year = 0;
  loadStudents(1);
}

function editStudent(item) {
  studentForm.id = Number(item.id || 0);
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
  if (!studentForm.student_no.trim() || !studentForm.full_name.trim()) {
    setError(new Error("学号和姓名不能为空"));
    return;
  }
  if (!studentForm.college_id || !studentForm.major_id) {
    setError(new Error("请先选择学院和专业"));
    return;
  }

  submitLoading.value = true;
  try {
    const payload = {
      student_no: studentForm.student_no.trim(),
      full_name: studentForm.full_name.trim(),
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
    await loadStudents(1);
  } catch (err) {
    setError(err);
  } finally {
    submitLoading.value = false;
  }
}

async function removeStudent(item) {
  if (!window.confirm(`确认删除学生「${item.full_name}」吗？`)) {
    return;
  }
  try {
    await api.delete(`/students/${item.id}`);
    setSuccess("学生删除成功");
    await loadStudents(pager.page);
  } catch (err) {
    setError(err);
  }
}

watch(
  () => queryForm.college_id,
  () => {
    if (!queryForm.major_id) {
      return;
    }
    const exist = queryMajors.value.some((m) => Number(m.id) === Number(queryForm.major_id));
    if (!exist) {
      queryForm.major_id = 0;
    }
  }
);

watch(
  () => studentForm.college_id,
  () => {
    if (!studentForm.major_id) {
      return;
    }
    const exist = formMajors.value.some((m) => Number(m.id) === Number(studentForm.major_id));
    if (!exist) {
      studentForm.major_id = 0;
    }
  }
);

onMounted(async () => {
  try {
    await loadBaseOptions();
    await loadStudents(1);
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
</style>
