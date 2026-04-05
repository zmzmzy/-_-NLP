<template>
  <section class="mc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="panel filter-panel">
      <div class="grid">
        <label>
          关键词
          <input v-model.trim="queryForm.keyword" placeholder="学院编码 / 名称 / 描述" @keyup.enter="search" />
        </label>
      </div>
      <div class="actions">
        <button class="secondary" :disabled="loading" @click="search">查询</button>
        <button class="secondary" :disabled="loading" @click="resetSearch">重置</button>
        <button @click="resetCollegeForm">新增学院</button>
      </div>
    </div>

    <div class="panel">
      <div class="section-head">
        <h3>学院列表</h3>
        <p class="meta">共 {{ pager.total_count }} 条，当前第 {{ pager.page }} / {{ pager.total_pages }} 页</p>
      </div>
      <table>
        <thead>
          <tr>
            <th>序号</th>
            <th>学院编码</th>
            <th>学院名称</th>
            <th>学院描述</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, index) in colleges" :key="item.id">
            <td>{{ rowNo(index) }}</td>
            <td>{{ item.code }}</td>
            <td>{{ item.name }}</td>
            <td>{{ item.description || '-' }}</td>
            <td class="row-actions">
              <button class="small secondary" @click="openMajors(item)">下属专业</button>
              <button class="small secondary" @click="editCollege(item)">编辑</button>
              <button class="small danger" @click="removeCollege(item)">删除</button>
            </td>
          </tr>
          <tr v-if="!colleges.length">
            <td colspan="5">暂无学院数据</td>
          </tr>
        </tbody>
      </table>
      <div class="actions page-actions">
        <button class="secondary" :disabled="loading || pager.page <= 1" @click="loadColleges(pager.page - 1)">上一页</button>
        <button class="secondary" :disabled="loading || pager.page >= pager.total_pages" @click="loadColleges(pager.page + 1)">下一页</button>
      </div>
    </div>

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
        <textarea v-model.trim="collegeForm.description" rows="3" placeholder="可选"></textarea>
      </label>
      <div class="actions">
        <button :disabled="submitLoading" @click="submitCollege">{{ submitLoading ? "提交中..." : (collegeForm.id ? "保存修改" : "新增学院") }}</button>
        <button class="secondary" :disabled="submitLoading" @click="resetCollegeForm">清空</button>
      </div>
    </div>
  </section>
</template>

<script setup>
import { onMounted, reactive, ref } from "vue";
import { useRouter } from "vue-router";
import api from "../../services/api";

const router = useRouter();

const loading = ref(false);
const submitLoading = ref(false);
const colleges = ref([]);
const errorText = ref("");
const successText = ref("");

const queryForm = reactive({
  keyword: ""
});

const pager = reactive({
  page: 1,
  page_size: 20,
  total_count: 0,
  total_pages: 1
});

const collegeForm = reactive({
  id: 0,
  code: "",
  name: "",
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

function resetCollegeForm() {
  collegeForm.id = 0;
  collegeForm.code = "";
  collegeForm.name = "";
  collegeForm.description = "";
}

function editCollege(item) {
  collegeForm.id = Number(item.id || 0);
  collegeForm.code = item.code || "";
  collegeForm.name = item.name || "";
  collegeForm.description = item.description || "";
}

function openMajors(item) {
  router.push({ name: "major-center-majors", query: { college_id: String(item.id) } });
}

async function loadColleges(page = 1) {
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
    const resp = await api.get("/colleges", { params });
    colleges.value = resp?.data?.data || [];
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
  loadColleges(1);
}

function resetSearch() {
  queryForm.keyword = "";
  loadColleges(1);
}

async function submitCollege() {
  if (!collegeForm.code.trim() || !collegeForm.name.trim()) {
    setError(new Error("学院编码和学院名称不能为空"));
    return;
  }
  submitLoading.value = true;
  try {
    const payload = {
      code: collegeForm.code.trim(),
      name: collegeForm.name.trim(),
      description: collegeForm.description.trim()
    };
    if (collegeForm.id) {
      await api.put(`/colleges/${collegeForm.id}`, payload);
      setSuccess("学院修改成功");
    } else {
      await api.post("/colleges", payload);
      setSuccess("学院新增成功");
    }
    resetCollegeForm();
    await loadColleges(1);
  } catch (err) {
    setError(err);
  } finally {
    submitLoading.value = false;
  }
}

async function removeCollege(item) {
  if (!window.confirm(`确认删除学院「${item.name}」吗？`)) {
    return;
  }
  try {
    await api.delete(`/colleges/${item.id}`);
    setSuccess("学院删除成功");
    await loadColleges(pager.page);
  } catch (err) {
    const status = Number(err?.response?.status || 0);
    const msg = err?.response?.data?.error || "";
    if (status === 409) {
      const forceConfirm = window.confirm(
        `${msg || "学院存在关联数据，无法直接删除。"}\n\n是否执行“强制删除”？\n强制删除会一并删除关联专业和学生数据。`
      );
      if (!forceConfirm) {
        setError(err);
        return;
      }
      try {
        await api.delete(`/colleges/${item.id}`, { params: { force: true } });
        setSuccess("学院强制删除成功");
        await loadColleges(pager.page);
      } catch (forceErr) {
        setError(forceErr);
      }
      return;
    }
    setError(err);
  }
}

onMounted(() => {
  loadColleges(1);
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
