<template>
  <section class="page gm-page fade-up">
    <h2>专业信息管理</h2>
    <p class="desc">
      本页已接入学院、专业、专业画像接口，可进行新增、编辑、删除与画像维护。
    </p>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">查询筛选</summary>
      <div class="panel-body">
        <div class="grid">
          <label>
            学院关键词
            <input v-model.trim="queryForm.college_keyword" placeholder="编码/名称/描述" />
          </label>
          <label>
            专业关键词
            <input v-model.trim="queryForm.major_keyword" placeholder="编码/名称/学院/描述" />
          </label>
          <label>
            按学院筛专业
            <select v-model.number="queryForm.major_college_id">
              <option :value="0">全部学院</option>
              <option v-for="c in colleges" :key="`q-college-${c.id}`" :value="c.id">{{ c.name }}</option>
            </select>
          </label>
        </div>
        <div class="actions">
          <button class="secondary" @click="applyQueryFilters">查询</button>
          <button class="secondary" @click="resetQueryFilters">重置</button>
        </div>
        <p class="hint">
          学院结果 {{ filteredColleges.length }}/{{ colleges.length }}，
          专业结果 {{ filteredMajors.length }}/{{ majors.length }}
        </p>
      </div>
    </details>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">专业导入（CSV 预检 + 应用）</summary>
      <div class="panel-body">
        <p class="hint">
          支持上传 CSV 或直接粘贴 CSV 文本。推荐表头：
          <code>university_code,university_name,college_code,college_name,major_code,major_name,major_description,core_courses_text,knowledge_skills_text,technical_platform_text,capability_text</code>
        </p>
        <div class="grid">
          <label>
            选择 CSV 文件
            <input type="file" accept=".csv,text/csv" @change="handleImportFileChange" />
          </label>
          <label>
            源文件名（可选）
            <input v-model.trim="importForm.source_filename" placeholder="如 majors_2026.csv" />
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
          <button :disabled="previewLoading" @click="previewMajorImport">
            {{ previewLoading ? "预检中..." : "预检导入" }}
          </button>
          <button
            class="secondary"
            :disabled="applyLoading || !importBatchId"
            @click="applyMajorImport"
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
              <th>专业编码</th>
              <th>专业名称</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="row in importRows" :key="row.row_no">
              <td>{{ row.row_no }}</td>
              <td>{{ row.row_status }}</td>
              <td>{{ row.plan_action || "-" }}</td>
              <td>{{ row.error_message || "-" }}</td>
              <td>{{ row.normalized_row?.major_code || "-" }}</td>
              <td>{{ row.normalized_row?.major_name || "-" }}</td>
            </tr>
          </tbody>
        </table>
      </div>
    </details>

    <details class="panel collapsible-panel">
      <summary class="panel-summary">导入批次历史</summary>
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
      <summary class="panel-summary">{{ collegeForm.id ? "编辑学院" : "新增学院" }}</summary>
      <div class="panel-body">
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
    </details>

    <div class="panel">
      <h3>学院列表</h3>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>编码</th>
            <th>名称</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in filteredColleges" :key="item.id">
            <td>{{ index + 1 }}</td>
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

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">{{ majorForm.id ? "编辑专业" : "新增专业" }}</summary>
      <div class="panel-body">
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
    </details>

    <div class="panel">
      <h3>专业列表</h3>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>专业编码</th>
            <th>专业名称</th>
            <th>学院</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in filteredMajors" :key="item.id">
            <td>{{ index + 1 }}</td>
            <td>{{ item.code }}</td>
            <td>{{ item.name }}</td>
            <td>{{ item.college_name }}</td>
            <td class="row-actions">
              <button class="small" @click="goMajorDetail(item)">详情</button>
              <button class="small secondary" @click="editMajor(item)">编辑</button>
              <button class="small secondary" @click="selectMajorForProfile(item)">画像</button>
              <button class="small danger" @click="removeMajor(item)">删除</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <details class="panel form-panel collapsible-panel">
      <summary class="panel-summary">专业画像维护</summary>
      <div class="panel-body">
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
    </details>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import { useRouter } from "vue-router";
import api from "../services/api";

const colleges = ref([]);
const majors = ref([]);
const profileMajorId = ref(0);
const importBatchId = ref(0);
const importSummary = ref(null);
const importRows = ref([]);
const importBatches = ref([]);
const previewLoading = ref(false);
const applyLoading = ref(false);
const historyLoading = ref(false);
const router = useRouter();

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
  description: ""
});

const profileForm = reactive({
  core_courses_text: "",
  knowledge_skills_text: "",
  technical_platform_text: "",
  capability_text: ""
});

const importForm = reactive({
  source_filename: "",
  csv_text: ""
});

const queryForm = reactive({
  college_keyword: "",
  major_keyword: "",
  major_college_id: 0
});

const activeQuery = reactive({
  college_keyword: "",
  major_keyword: "",
  major_college_id: 0
});

const filteredColleges = computed(() => {
  const keyword = activeQuery.college_keyword.trim().toLowerCase();
  if (!keyword) {
    return colleges.value;
  }
  return colleges.value.filter((item) => {
    const searchText = `${item.code || ""} ${item.name || ""} ${item.description || ""}`.toLowerCase();
    return searchText.includes(keyword);
  });
});

const filteredMajors = computed(() => {
  const keyword = activeQuery.major_keyword.trim().toLowerCase();
  const collegeId = Number(activeQuery.major_college_id || 0);
  return majors.value.filter((item) => {
    if (collegeId > 0 && Number(item.college_id || 0) !== collegeId) {
      return false;
    }
    if (!keyword) {
      return true;
    }
    const searchText = `${item.code || ""} ${item.name || ""} ${item.college_name || ""} ` +
      `${item.description || ""}`.toLowerCase();
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

function resetImportPreview() {
  importBatchId.value = 0;
  importSummary.value = null;
  importRows.value = [];
}

function applyQueryFilters() {
  activeQuery.college_keyword = queryForm.college_keyword;
  activeQuery.major_keyword = queryForm.major_keyword;
  activeQuery.major_college_id = Number(queryForm.major_college_id || 0);
}

function resetQueryFilters() {
  queryForm.college_keyword = "";
  queryForm.major_keyword = "";
  queryForm.major_college_id = 0;
  applyQueryFilters();
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
    await loadMajors();
  } catch (err) {
    const status = Number(err?.response?.status || 0);
    const msg = err?.response?.data?.error || "";
    if (status === 409) {
      const forceConfirm = window.confirm(
        `${msg || "学院存在关联数据，无法直接删除。"}\n\n是否执行“强制删除”？\n` +
          "强制删除会一并删除该学院下的专业，以及相关学生/匹配数据（仅管理员可执行）。"
      );
      if (!forceConfirm) {
        setError(err);
        return;
      }
      try {
        await api.delete(`/colleges/${item.id}`, { params: { force: true } });
        setSuccess("学院强制删除成功");
        await loadColleges();
        await loadMajors();
        return;
      } catch (forceErr) {
        setError(forceErr);
        return;
      }
    }
    setError(err);
  }
}

function editMajor(item) {
  majorForm.id = item.id;
  majorForm.college_id = Number(item.college_id || 0);
  majorForm.code = item.code || "";
  majorForm.name = item.name || "";
  majorForm.description = item.description || "";
}

function goMajorDetail(item) {
  if (!item?.id) {
    return;
  }
  router.push(`/majors/${item.id}`);
}

async function submitMajor() {
  try {
    const payload = {
      college_id: Number(majorForm.college_id),
      code: majorForm.code,
      name: majorForm.name,
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

async function previewMajorImport() {
  if (!importForm.csv_text.trim()) {
    setError(new Error("请先选择或粘贴 CSV 内容"));
    return;
  }
  previewLoading.value = true;
  try {
    const resp = await api.post("/majors/import/preview", {
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

async function applyMajorImport() {
  if (!importBatchId.value) {
    setError(new Error("请先执行预检"));
    return;
  }
  applyLoading.value = true;
  try {
    const resp = await api.post("/majors/import/apply", { batch_id: importBatchId.value });
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
    await loadColleges();
    await loadMajors();
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
    const resp = await api.get("/majors/import/batches", {
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
    await loadColleges();
    await loadMajors();
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
