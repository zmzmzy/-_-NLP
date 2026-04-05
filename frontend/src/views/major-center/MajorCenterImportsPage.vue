<template>
  <section class="mc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="cards">
      <article class="panel import-card">
        <h3>学院导入</h3>
        <p class="hint">支持模板下载、预检、错误报告后再确认入库。</p>
        <p class="meta">状态：接口待接入</p>
        <button class="secondary" disabled>即将支持</button>
      </article>
      <article class="panel import-card active">
        <h3>专业导入</h3>
        <p class="hint">已接入 CSV 预检、应用、历史批次查看。</p>
        <p class="meta">状态：可用</p>
        <button class="secondary" @click="scrollToMajorImport">定位到操作区</button>
      </article>
      <article class="panel import-card">
        <h3>学生导入</h3>
        <p class="hint">建议接入校验：学号唯一、学院专业映射、年级格式。</p>
        <p class="meta">状态：接口待接入</p>
        <button class="secondary" disabled>即将支持</button>
      </article>
    </div>

    <div ref="majorImportRef" class="panel form-panel">
      <h3>专业导入（CSV 预检 + 应用）</h3>
      <p class="hint">
        推荐表头：
        <code>
          university_code,university_name,college_code,college_name,major_code,major_name,major_description,core_courses_text,knowledge_skills_text,technical_platform_text,capability_text
        </code>
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
          rows="8"
          placeholder="可直接粘贴 CSV 内容，或通过上方文件选择自动填充"
        ></textarea>
      </label>
      <div class="actions">
        <button :disabled="previewLoading" @click="previewMajorImport">
          {{ previewLoading ? "预检中..." : "预检导入" }}
        </button>
        <button class="secondary" :disabled="applyLoading || !importBatchId" @click="applyMajorImport">
          {{ applyLoading ? "应用中..." : "确认应用" }}
        </button>
      </div>
      <p v-if="importSummary" class="meta">
        当前预检批次：#{{ importBatchId }}，总行 {{ importSummary.total_rows }}，
        有效 {{ importSummary.valid_rows }}，无效 {{ importSummary.invalid_rows }}，
        预计新增 {{ importSummary.estimated_inserted_rows }}，预计更新 {{ importSummary.estimated_updated_rows }}
      </p>
    </div>

    <div v-if="importRows.length" class="panel">
      <h3>预检明细</h3>
      <table>
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
            <td>{{ row.plan_action || '-' }}</td>
            <td>{{ row.error_message || '-' }}</td>
            <td>{{ row.normalized_row?.major_code || '-' }}</td>
            <td>{{ row.normalized_row?.major_name || '-' }}</td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="panel">
      <div class="section-head">
        <h3>导入批次历史</h3>
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
          <tr v-if="!importBatches.length">
            <td colspan="11">暂无导入批次</td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>

<script setup>
import { onMounted, reactive, ref } from "vue";
import api from "../../services/api";

const majorImportRef = ref(null);
const importBatchId = ref(0);
const importSummary = ref(null);
const importRows = ref([]);
const importBatches = ref([]);

const previewLoading = ref(false);
const applyLoading = ref(false);
const historyLoading = ref(false);

const errorText = ref("");
const successText = ref("");

const importForm = reactive({
  source_filename: "",
  csv_text: ""
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

function scrollToMajorImport() {
  majorImportRef.value?.scrollIntoView({ behavior: "smooth", block: "start" });
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
      setSuccess(`导入应用完成，批次 #${importBatchId.value} 未产生变更（跳过 ${skipped} 行）`);
    } else {
      setSuccess(`导入应用成功，批次 #${importBatchId.value}：新增 ${inserted}，更新 ${updated}，跳过 ${skipped}`);
    }
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

onMounted(() => {
  loadImportBatches();
});
</script>

<style scoped>
.mc-view {
  display: grid;
  gap: 12px;
}

.cards {
  display: grid;
  gap: 12px;
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.import-card {
  display: grid;
  gap: 8px;
}

.import-card h3 {
  margin: 0;
}

.import-card.active {
  border-color: rgba(125, 211, 252, 0.44);
}

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

@media (max-width: 1000px) {
  .cards {
    grid-template-columns: 1fr;
  }
}
</style>
