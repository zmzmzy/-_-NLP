<template>
  <section class="jc-view fade-up">
    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <div class="cards">
      <article class="panel import-card active">
        <h3>岗位导入</h3>
        <p class="hint">已接入 CSV 预检、应用与批次历史查询，建议先预检再入库。</p>
        <p class="meta">状态：可用</p>
        <button class="secondary" @click="scrollToImportArea">定位到操作区</button>
      </article>
      <article class="panel import-card">
        <h3>企业导入</h3>
        <p class="hint">建议后续补充企业导入接口，与岗位导入形成联动校验。</p>
        <p class="meta">状态：接口待接入</p>
        <button class="secondary" disabled>即将支持</button>
      </article>
      <article class="panel import-card">
        <h3>质量提示</h3>
        <p class="hint">建议优先修复无效行，避免专业字段缺失影响后续匹配分析。</p>
        <p class="meta">建议：先预检，再应用</p>
        <button class="secondary" :disabled="historyLoading" @click="loadImportBatches">
          {{ historyLoading ? "刷新中..." : "刷新历史" }}
        </button>
      </article>
    </div>

    <div ref="importAreaRef" class="panel form-panel">
      <h3>岗位导入（CSV 预检 + 应用）</h3>
      <p class="hint">
        推荐表头：
        <code>
          company_name,title,job_type,min_degree,location_province,location_city,salary_range,responsibilities_text,technical_projects_text,required_knowledge_skills_text,required_tools_platform_text,published_at,domain_area,main_business,flagship_products,application_industries,company_province,company_city,company_website,company_description
        </code>
      </p>
      <div class="grid">
        <label>
          选择 CSV 文件
          <input type="file" accept=".csv,text/csv" @change="handleImportFileChange" />
        </label>
        <label>
          源文件名（可选）
          <input v-model.trim="importForm.source_filename" placeholder="如 jobs_2026.csv" />
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
        <button :disabled="previewLoading" @click="previewJobImport">
          {{ previewLoading ? "预检中..." : "预检导入" }}
        </button>
        <button class="secondary" :disabled="applyLoading || !importBatchId" @click="applyJobImport">
          {{ applyLoading ? "应用中..." : "确认应用" }}
        </button>
        <button class="secondary" :disabled="previewLoading || applyLoading" @click="resetImportForm">
          清空
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
            <th>企业</th>
            <th>岗位</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="row in importRows" :key="row.row_no">
            <td>{{ row.row_no }}</td>
            <td>{{ row.row_status }}</td>
            <td>{{ row.plan_action || "-" }}</td>
            <td>{{ row.error_message || "-" }}</td>
            <td>{{ row.normalized_row?.company_name || "-" }}</td>
            <td>{{ row.normalized_row?.title || "-" }}</td>
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

const importAreaRef = ref(null);
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

function resetImportForm() {
  importForm.source_filename = "";
  importForm.csv_text = "";
  resetImportPreview();
}

function scrollToImportArea() {
  importAreaRef.value?.scrollIntoView({ behavior: "smooth", block: "start" });
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

async function previewJobImport() {
  if (!importForm.csv_text.trim()) {
    setError(new Error("请先选择或粘贴 CSV 内容"));
    return;
  }
  previewLoading.value = true;
  try {
    const resp = await api.post("/jobs/import/preview", {
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

async function applyJobImport() {
  if (!importBatchId.value) {
    setError(new Error("请先执行预检"));
    return;
  }
  applyLoading.value = true;
  try {
    const resp = await api.post("/jobs/import/apply", { batch_id: importBatchId.value });
    const result = resp?.data?.data || {};
    const inserted = Number(result.inserted_rows || 0);
    const updated = Number(result.updated_rows || 0);
    const skipped = Number(result.skipped_rows || 0);
    if (inserted === 0 && updated === 0) {
      setSuccess(`导入应用完成，批次 #${importBatchId.value} 未产生数据变更（跳过 ${skipped} 行）`);
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
    const resp = await api.get("/jobs/import/batches", {
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
.jc-view {
  display: grid;
  gap: 12px;
}

.import-card {
  padding: 12px;
}

.import-card h3 {
  margin: 0 0 8px;
}

.import-card.active {
  border-color: rgba(56, 189, 248, 0.52);
  background: rgba(14, 165, 233, 0.14);
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
</style>
