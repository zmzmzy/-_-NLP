<template>
  <section class="page">
    <h2>{{ isAdmin ? "管理员个人主页 / 就业审核" : "个人主页" }}</h2>
    <p class="desc">
      {{
        isAdmin
          ? "管理员可在此审核学生提交的就业信息，单条或批量确认后写入正式就业记录。"
          : "在此维护个人学生信息，并提交就业信息（需管理员审核后生效）。"
      }}
    </p>

    <p v-if="errorText" class="error">{{ errorText }}</p>
    <p v-if="successText" class="success">{{ successText }}</p>

    <template v-if="isAdmin">
      <details class="panel form-panel collapsible-panel" open>
        <summary class="panel-summary">审核筛选</summary>
        <div class="panel-body">
          <div class="grid">
            <label>
              状态
              <select v-model="adminFilters.status">
                <option value="">全部</option>
                <option value="pending">pending</option>
                <option value="approved">approved</option>
                <option value="rejected">rejected</option>
              </select>
            </label>
            <label>
              关键词
              <input v-model.trim="adminFilters.keyword" placeholder="学生名/企业/岗位/提交人" />
            </label>
            <label>
              排序
              <select v-model="adminFilters.sort_order">
                <option value="desc">最新优先</option>
                <option value="asc">最早优先</option>
              </select>
            </label>
          </div>
          <div class="actions">
            <button :disabled="adminLoading" @click="searchAdminSubmissions">
              {{ adminLoading ? "查询中..." : "查询" }}
            </button>
            <button class="secondary" :disabled="adminLoading" @click="resetAdminFilters">
              重置
            </button>
            <button class="secondary" :disabled="exporting" @click="exportAdminSubmissions">
              {{ exporting ? "导出中..." : "导出当前筛选CSV" }}
            </button>
          </div>
        </div>
      </details>

      <details class="panel form-panel collapsible-panel" open>
        <summary class="panel-summary">批量处理</summary>
        <div class="panel-body">
          <div class="grid">
            <label>
              选中条数
              <input :value="selectedIds.length" disabled />
            </label>
            <label>
              审核备注（可选）
              <input v-model.trim="batchReviewComment" placeholder="批量通过备注" />
            </label>
          </div>
          <div class="actions">
            <button :disabled="batchApproving || !selectedIds.length" @click="approveSelected">
              {{ batchApproving ? "处理中..." : "批量通过" }}
            </button>
            <button class="danger" :disabled="batchRejecting || !selectedIds.length" @click="rejectSelected">
              {{ batchRejecting ? "处理中..." : "批量拒绝" }}
            </button>
            <button class="secondary" :disabled="!selectedIds.length" @click="clearSelection">
              清空选择
            </button>
          </div>
        </div>
      </details>

      <details class="panel collapsible-panel" open>
        <summary class="panel-summary">最近审核操作日志</summary>
        <div class="panel-body">
          <div class="actions">
            <button class="secondary" :disabled="logLoading" @click="loadAdminLogs">
              {{ logLoading ? "刷新中..." : "刷新日志" }}
            </button>
          </div>
          <table>
            <thead>
              <tr>
                <th>序号</th>
                <th>时间</th>
                <th>操作人</th>
                <th>动作</th>
                <th>状态码</th>
                <th>来源IP</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="(log, index) in adminLogs" :key="`admin-log-${log.id}`">
                <td>{{ index + 1 }}</td>
                <td>{{ log.created_at }}</td>
                <td>{{ log.actor_username || log.actor_user_id || "-" }}</td>
                <td>{{ log.action }}</td>
                <td>{{ log.details?.status_code || "-" }}</td>
                <td>{{ log.ip_address || "-" }}</td>
              </tr>
              <tr v-if="!adminLogs.length">
                <td colspan="6">暂无日志</td>
              </tr>
            </tbody>
          </table>
        </div>
      </details>

      <div class="panel">
        <h3>待审核列表</h3>
        <p class="meta">
          当前第 {{ adminMeta.page }} / {{ adminMeta.total_pages }} 页，共 {{ adminMeta.total_count }} 条
        </p>
        <table>
          <thead>
            <tr>
              <th>
                <input
                  type="checkbox"
                  :checked="allPendingChecked"
                  :disabled="!pendingIdsInPage.length"
                  @change="toggleSelectAll($event)"
                />
              </th>
              <th>序号</th>
              <th>状态</th>
              <th>学生</th>
              <th>企业 / 岗位</th>
              <th>提交人</th>
              <th>创建时间</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(item, index) in adminSubmissions" :key="`admin-sub-${item.id}`">
              <td>
                <input
                  type="checkbox"
                  :checked="selectedIdSet.has(Number(item.id))"
                  :disabled="item.status !== 'pending'"
                  @change="toggleSelectOne(item.id, $event)"
                />
              </td>
              <td>{{ adminSubmissionNo(index) }}</td>
              <td>{{ item.status }}</td>
              <td>{{ item.student_name }}（#{{ item.student_id }}）</td>
              <td>
                {{ item.company_name || "-" }}
                <span class="meta-tag">{{ item.company_source === "proposed" ? "候选" : "库内" }}</span>
                /
                {{ item.job_title || "-" }}
                <span v-if="item.job_source === 'proposed'" class="meta-tag">候选</span>
              </td>
              <td>{{ item.submitter_username || item.submitter_user_id }}</td>
              <td>{{ item.created_at }}</td>
              <td class="row-actions">
                <button class="small" :disabled="item.status !== 'pending'" @click="approveOne(item)">
                  通过
                </button>
                <button class="small danger" :disabled="item.status !== 'pending'" @click="rejectOne(item)">
                  拒绝
                </button>
              </td>
            </tr>
            <tr v-if="!adminSubmissions.length">
              <td colspan="8">暂无记录</td>
            </tr>
          </tbody>
        </table>
        <div class="actions">
          <button class="secondary" :disabled="adminMeta.page <= 1 || adminLoading" @click="changeAdminPage(adminMeta.page - 1)">
            上一页
          </button>
          <button
            class="secondary"
            :disabled="adminMeta.page >= adminMeta.total_pages || adminLoading"
            @click="changeAdminPage(adminMeta.page + 1)"
          >
            下一页
          </button>
        </div>
      </div>
    </template>

    <template v-else>
      <details class="panel collapsible-panel" open>
        <summary class="panel-summary">我的账户</summary>
        <div class="panel-body">
          <p class="meta">账号：{{ authStore.user?.username || "-" }}</p>
          <p class="meta">邮箱：{{ authStore.user?.email || "-" }}</p>
          <p class="meta">权限：{{ authStore.user?.role || "viewer" }}</p>
          <p class="meta">学生绑定状态：{{ hasBoundStudent ? "已绑定" : "未绑定" }}</p>
        </div>
      </details>

      <details class="panel form-panel collapsible-panel" :open="!hasBoundStudent">
        <summary class="panel-summary">学生身份绑定（学号 + 姓名）</summary>
        <div class="panel-body">
          <p class="meta">
            {{ hasBoundStudent ? "已完成绑定，可维护个人资料与就业提交。" : "请先绑定学号和姓名，绑定成功后才能修改与提交。"}}
          </p>
          <div v-if="!hasBoundStudent" class="grid compact-grid">
            <label>
              学号
              <input v-model.trim="bindForm.student_no" placeholder="请输入学号" />
            </label>
            <label>
              姓名
              <input v-model.trim="bindForm.full_name" placeholder="请输入姓名" />
            </label>
          </div>
          <div v-if="!hasBoundStudent" class="actions">
            <button :disabled="bindingStudent" @click="bindStudentIdentity">
              {{ bindingStudent ? "绑定中..." : "绑定学生身份" }}
            </button>
            <button class="secondary" :disabled="profileLoading" @click="loadMyProfile">
              {{ profileLoading ? "加载中..." : "刷新状态" }}
            </button>
          </div>
        </div>
      </details>

      <details v-if="hasBoundStudent" class="panel form-panel collapsible-panel" open>
        <summary class="panel-summary">学生资料维护</summary>
        <div class="panel-body">
          <div class="grid">
            <label>
              学号
              <input v-model.trim="profileForm.student_no" disabled />
            </label>
            <label>
              姓名
              <input v-model.trim="profileForm.full_name" disabled />
            </label>
            <label>
              性别
              <select v-model="profileForm.gender">
                <option value="male">男</option>
                <option value="female">女</option>
                <option value="other">其他</option>
                <option value="unknown">未知</option>
              </select>
            </label>
            <label>
              毕业年份
              <input v-model.number="profileForm.graduation_year" type="number" min="2000" max="2100" disabled />
            </label>
            <label>
              学院
              <select v-model.number="profileForm.college_id" disabled>
                <option :value="0">请选择学院</option>
                <option v-for="c in colleges" :key="`profile-college-${c.id}`" :value="Number(c.id)">{{ c.name }}</option>
              </select>
            </label>
            <label>
              专业
              <select v-model.number="profileForm.major_id" disabled>
                <option :value="0">请选择专业</option>
                <option
                  v-for="m in filteredMajors"
                  :key="`profile-major-${m.id}`"
                  :value="Number(m.id)"
                >
                  {{ m.name }}（{{ m.code }}）
                </option>
              </select>
            </label>
            <label>
              邮箱
              <input v-model.trim="profileForm.email" placeholder="可选" />
            </label>
            <label>
              手机号
              <input v-model.trim="profileForm.phone" placeholder="可选" />
            </label>
          </div>

          <label>
            简历文本（专业知识与技能）
            <textarea v-model="profileForm.resume_text" rows="4" placeholder="填写所学课程、专业知识、技能"></textarea>
          </label>
          <label>
            技术项目经历
            <textarea v-model="profileForm.technical_projects_text" rows="3" placeholder="填写在校项目或实习项目"></textarea>
          </label>
          <label>
            能力总结
            <textarea v-model="profileForm.capability_summary_text" rows="3" placeholder="例如沟通协作、工程实践、问题解决"></textarea>
          </label>

          <div class="actions">
            <button :disabled="profileSaving" @click="saveProfile">
              {{ profileSaving ? "保存中..." : "保存可编辑资料" }}
            </button>
            <button class="secondary" :disabled="profileLoading" @click="loadMyProfile">
              {{ profileLoading ? "加载中..." : "刷新资料" }}
            </button>
          </div>
        </div>
      </details>

      <details v-if="hasBoundStudent" class="panel form-panel collapsible-panel" open>
        <summary class="panel-summary">提交就业信息（待审核）</summary>
        <div class="panel-body">
          <div class="grid">
            <label>
              提交模式
              <select v-model="submissionMode">
                <option value="existing">已有公司/岗位</option>
                <option value="new">新公司/新岗位</option>
              </select>
            </label>
            <template v-if="submissionMode === 'existing'">
              <label>
                就业公司（已有）
                <select v-model.number="submissionForm.company_id">
                  <option :value="0">请选择公司</option>
                  <option v-for="c in companies" :key="`sub-company-${c.id}`" :value="Number(c.id)">{{ c.name }}</option>
                </select>
              </label>
              <label>
                就业岗位（已有，可选）
                <select v-model.number="submissionForm.job_id">
                  <option :value="0">不绑定岗位</option>
                  <option v-for="j in filteredJobs" :key="`sub-job-${j.id}`" :value="Number(j.id)">
                    {{ j.title }}（{{ j.company_name }}）
                  </option>
                </select>
              </label>
            </template>
            <template v-else>
              <label>
                新公司名称
                <input v-model.trim="submissionForm.proposed_company_name" placeholder="请输入公司名称" />
              </label>
              <label>
                新岗位名称（可选）
                <input v-model.trim="submissionForm.proposed_job_title" placeholder="请输入岗位名称" />
              </label>
              <label>
                公司主业（可选）
                <input v-model.trim="submissionForm.proposed_company_main_business" placeholder="如：工业软件研发" />
              </label>
              <label>
                拳头产品（可选）
                <input v-model.trim="submissionForm.proposed_company_flagship_products" placeholder="如：xxx平台" />
              </label>
              <label>
                业务领域（可选）
                <input v-model.trim="submissionForm.proposed_company_domain_area" placeholder="如：智能制造" />
              </label>
              <label>
                应用行业（可选）
                <input v-model.trim="submissionForm.proposed_company_application_industries" placeholder="如：汽车、能源" />
              </label>
            </template>
            <label>
              就业类型
              <select v-model="submissionForm.employment_type">
                <option value="signed">签约</option>
                <option value="offer">offer</option>
                <option value="internship">实习</option>
                <option value="pending">待定</option>
              </select>
            </label>
            <label>
              相关材料
              <input v-model.trim="submissionForm.evidence_source" placeholder="如 offer 邮件/三方协议" />
            </label>
          </div>
          <div v-if="submissionMode === 'new'" class="grid">
            <label>
              新岗位职责描述（可选）
              <textarea v-model="submissionForm.proposed_job_responsibilities_text" rows="2" placeholder="岗位职责关键描述"></textarea>
            </label>
            <label>
              新岗位技术项目（可选）
              <textarea v-model="submissionForm.proposed_job_technical_projects_text" rows="2" placeholder="可能参与的技术项目"></textarea>
            </label>
            <label>
              新岗位知识技能要求（可选）
              <textarea v-model="submissionForm.proposed_job_required_knowledge_skills_text" rows="2" placeholder="岗位要求知识与技能"></textarea>
            </label>
            <label>
              新岗位工具平台要求（可选）
              <textarea v-model="submissionForm.proposed_job_required_tools_platform_text" rows="2" placeholder="如 C++/Qt/Linux"></textarea>
            </label>
          </div>
          <label>
            备注
            <textarea v-model="submissionForm.notes" rows="3" placeholder="补充岗位项目、技术要求等"></textarea>
          </label>

          <div class="actions">
            <button :disabled="submissionSaving" @click="submitEmployment">
              {{ submissionSaving ? "提交中..." : "提交审核" }}
            </button>
            <button class="secondary" @click="resetSubmissionForm">重置表单</button>
          </div>
        </div>
      </details>

      <div v-if="hasBoundStudent" class="panel">
        <h3>我的就业提交历史</h3>
        <details class="form-panel collapsible-panel">
          <summary class="panel-summary">筛选</summary>
          <div class="panel-body">
            <div class="grid compact-grid">
              <label>
                状态
                <select v-model="mySubmissionStatus">
                  <option value="">全部</option>
                  <option value="pending">pending</option>
                  <option value="approved">approved</option>
                  <option value="rejected">rejected</option>
                </select>
              </label>
            </div>
            <div class="actions">
              <button class="secondary" :disabled="mySubmissionLoading" @click="loadMySubmissions(1)">
                {{ mySubmissionLoading ? "查询中..." : "查询" }}
              </button>
            </div>
          </div>
        </details>

        <p class="meta">
          当前第 {{ mySubMeta.page }} / {{ mySubMeta.total_pages }} 页，共 {{ mySubMeta.total_count }} 条
        </p>
        <table>
          <thead>
            <tr>
              <th>序号</th>
              <th>状态</th>
              <th>公司 / 岗位</th>
              <th>就业类型</th>
              <th>审核备注</th>
              <th>创建时间</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(item, index) in mySubmissions" :key="`my-sub-${item.id}`">
              <td>{{ mySubmissionNo(index) }}</td>
              <td>{{ item.status }}</td>
              <td>
                {{ item.company_name || "-" }}
                <span class="meta-tag">{{ item.company_source === "proposed" ? "候选" : "库内" }}</span>
                /
                {{ item.job_title || "-" }}
                <span v-if="item.job_source === 'proposed'" class="meta-tag">候选</span>
              </td>
              <td>{{ item.employment_type }} / {{ item.employment_status }}</td>
              <td>{{ item.review_comment || "-" }}</td>
              <td>{{ item.created_at }}</td>
            </tr>
            <tr v-if="!mySubmissions.length">
              <td colspan="6">暂无记录</td>
            </tr>
          </tbody>
        </table>
        <div class="actions">
          <button class="secondary" :disabled="mySubMeta.page <= 1 || mySubmissionLoading" @click="loadMySubmissions(mySubMeta.page - 1)">
            上一页
          </button>
          <button
            class="secondary"
            :disabled="mySubMeta.page >= mySubMeta.total_pages || mySubmissionLoading"
            @click="loadMySubmissions(mySubMeta.page + 1)"
          >
            下一页
          </button>
        </div>
      </div>
    </template>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref, watch } from "vue";
import api from "../services/api";
import { useAuthStore } from "../stores/auth";

const authStore = useAuthStore();
const isAdmin = computed(() => authStore.user?.role === "admin");

const errorText = ref("");
const successText = ref("");

const profileLoading = ref(false);
const profileSaving = ref(false);
const bindingStudent = ref(false);
const submissionSaving = ref(false);
const mySubmissionLoading = ref(false);
const adminLoading = ref(false);
const batchApproving = ref(false);
const batchRejecting = ref(false);
const exporting = ref(false);
const logLoading = ref(false);

const colleges = ref([]);
const majors = ref([]);
const companies = ref([]);
const jobs = ref([]);
const boundStudentId = ref(0);

const bindForm = reactive({
  student_no: "",
  full_name: ""
});

const profileForm = reactive({
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

const submissionForm = reactive({
  company_id: 0,
  job_id: 0,
  proposed_company_name: "",
  proposed_company_main_business: "",
  proposed_company_flagship_products: "",
  proposed_company_domain_area: "",
  proposed_company_application_industries: "",
  proposed_job_title: "",
  proposed_job_responsibilities_text: "",
  proposed_job_technical_projects_text: "",
  proposed_job_required_knowledge_skills_text: "",
  proposed_job_required_tools_platform_text: "",
  employment_type: "signed",
  employment_status: "active",
  start_date: "",
  end_date: "",
  evidence_source: "",
  notes: ""
});
const submissionMode = ref("existing");

const mySubmissions = ref([]);
const mySubmissionStatus = ref("");
const mySubMeta = reactive({
  page: 1,
  page_size: 20,
  total_count: 0,
  total_pages: 1
});

const adminFilters = reactive({
  status: "pending",
  keyword: "",
  sort_order: "desc",
  page: 1,
  page_size: 20
});
const adminSubmissions = ref([]);
const adminLogs = ref([]);
const adminMeta = reactive({
  page: 1,
  page_size: 20,
  total_count: 0,
  total_pages: 1
});
const selectedIds = ref([]);
const batchReviewComment = ref("");

const selectedIdSet = computed(() => new Set(selectedIds.value.map((id) => Number(id))));
const hasBoundStudent = computed(() => Number(boundStudentId.value || 0) > 0);
const filteredMajors = computed(() => {
  if (!profileForm.college_id) {
    return majors.value;
  }
  return majors.value.filter((m) => Number(m.college_id) === Number(profileForm.college_id));
});
const filteredJobs = computed(() => {
  if (!submissionForm.company_id) {
    return jobs.value;
  }
  return jobs.value.filter((j) => Number(j.company_id) === Number(submissionForm.company_id));
});
const pendingIdsInPage = computed(() =>
  adminSubmissions.value
    .filter((item) => item.status === "pending")
    .map((item) => Number(item.id))
);
const allPendingChecked = computed(() => {
  if (!pendingIdsInPage.value.length) {
    return false;
  }
  return pendingIdsInPage.value.every((id) => selectedIdSet.value.has(id));
});

watch(
  () => profileForm.college_id,
  () => {
    if (!profileForm.major_id) {
      return;
    }
    const found = filteredMajors.value.some((m) => Number(m.id) === Number(profileForm.major_id));
    if (!found) {
      profileForm.major_id = 0;
    }
  }
);

watch(
  () => submissionForm.company_id,
  () => {
    if (!submissionForm.job_id) {
      return;
    }
    const found = filteredJobs.value.some((j) => Number(j.id) === Number(submissionForm.job_id));
    if (!found) {
      submissionForm.job_id = 0;
    }
  }
);

watch(
  () => submissionMode.value,
  (mode) => {
    if (mode === "existing") {
      submissionForm.proposed_company_name = "";
      submissionForm.proposed_company_main_business = "";
      submissionForm.proposed_company_flagship_products = "";
      submissionForm.proposed_company_domain_area = "";
      submissionForm.proposed_company_application_industries = "";
      submissionForm.proposed_job_title = "";
      submissionForm.proposed_job_responsibilities_text = "";
      submissionForm.proposed_job_technical_projects_text = "";
      submissionForm.proposed_job_required_knowledge_skills_text = "";
      submissionForm.proposed_job_required_tools_platform_text = "";
    } else {
      submissionForm.company_id = 0;
      submissionForm.job_id = 0;
    }
  }
);

function clearFeedback() {
  errorText.value = "";
  successText.value = "";
}

function pickError(err, fallback) {
  return err?.response?.data?.error || err?.message || fallback;
}

function applyMeta(target, meta) {
  target.page = Number(meta?.page || 1);
  target.page_size = Number(meta?.page_size || 20);
  target.total_count = Number(meta?.total_count || 0);
  target.total_pages = Number(meta?.total_pages || 1);
}

function displayRowNo(index, page, pageSize) {
  const p = Math.max(1, Number(page || 1));
  const ps = Math.max(1, Number(pageSize || 20));
  return (p - 1) * ps + Number(index) + 1;
}

function adminSubmissionNo(index) {
  return displayRowNo(index, adminMeta.page, adminMeta.page_size);
}

function mySubmissionNo(index) {
  return displayRowNo(index, mySubMeta.page, mySubMeta.page_size);
}

function applyProfileStudent(student) {
  profileForm.student_no = student?.student_no || "";
  profileForm.full_name = student?.full_name || "";
  profileForm.gender = student?.gender || "unknown";
  profileForm.college_id = Number(student?.college_id || 0);
  profileForm.major_id = Number(student?.major_id || 0);
  profileForm.graduation_year = Number(student?.graduation_year || new Date().getFullYear());
  profileForm.email = student?.email || "";
  profileForm.phone = student?.phone || "";
  profileForm.resume_text = student?.resume_text || "";
  profileForm.technical_projects_text = student?.technical_projects_text || "";
  profileForm.capability_summary_text = student?.capability_summary_text || "";
}

async function loadStaticOptions() {
  const [collegeResp, majorResp, companyResp, jobResp] = await Promise.all([
    api.get("/colleges", { params: { page: 1, page_size: 500, sort_by: "id", sort_order: "desc" } }),
    api.get("/majors", { params: { page: 1, page_size: 1000, sort_by: "id", sort_order: "desc" } }),
    api.get("/companies", { params: { page: 1, page_size: 1000, sort_by: "id", sort_order: "desc" } }),
    api.get("/jobs", { params: { page: 1, page_size: 1000, sort_by: "id", sort_order: "desc" } })
  ]);
  colleges.value = collegeResp?.data?.data || [];
  majors.value = majorResp?.data?.data || [];
  companies.value = companyResp?.data?.data || [];
  jobs.value = jobResp?.data?.data || [];
}

async function loadMyProfile() {
  clearFeedback();
  profileLoading.value = true;
  try {
    const resp = await api.get("/me/student-profile");
    const data = resp?.data?.data || {};
    boundStudentId.value = Number(data?.user?.linked_student_id || 0);
    applyProfileStudent(data.student || null);
    if (boundStudentId.value > 0) {
      bindForm.student_no = profileForm.student_no;
      bindForm.full_name = profileForm.full_name;
    }
    await authStore.fetchMe();
  } catch (err) {
    errorText.value = pickError(err, "加载个人资料失败");
  } finally {
    profileLoading.value = false;
  }
}

async function bindStudentIdentity() {
  clearFeedback();
  if (hasBoundStudent.value) {
    successText.value = "当前账号已完成学生身份绑定";
    return;
  }
  if (!bindForm.student_no || !bindForm.full_name) {
    errorText.value = "学号和姓名不能为空";
    return;
  }
  bindingStudent.value = true;
  try {
    const resp = await api.post("/me/student-profile/bind", {
      student_no: bindForm.student_no,
      full_name: bindForm.full_name
    });
    const data = resp?.data?.data || {};
    boundStudentId.value = Number(data?.linked_student_id || 0);
    applyProfileStudent(data.student || null);
    await authStore.fetchMe();
    successText.value = data?.already_bound ? "账号已绑定该学生身份" : "学生身份绑定成功";
    if (hasBoundStudent.value) {
      await loadMySubmissions(1);
      resetSubmissionForm();
    }
  } catch (err) {
    errorText.value = pickError(err, "绑定学生身份失败");
  } finally {
    bindingStudent.value = false;
  }
}

async function saveProfile() {
  clearFeedback();
  if (!hasBoundStudent.value) {
    errorText.value = "请先绑定学生身份";
    return;
  }
  profileSaving.value = true;
  try {
    const payload = {
      gender: profileForm.gender,
      email: profileForm.email || "",
      phone: profileForm.phone || "",
      resume_text: profileForm.resume_text || "",
      technical_projects_text: profileForm.technical_projects_text || "",
      capability_summary_text: profileForm.capability_summary_text || ""
    };
    const resp = await api.put("/me/student-profile", payload);
    const student = resp?.data?.data?.student || null;
    applyProfileStudent(student);
    await authStore.fetchMe();
    successText.value = "学生资料已保存";
  } catch (err) {
    errorText.value = pickError(err, "保存学生资料失败");
  } finally {
    profileSaving.value = false;
  }
}

function resetSubmissionForm() {
  submissionMode.value = "existing";
  submissionForm.company_id = 0;
  submissionForm.job_id = 0;
  submissionForm.proposed_company_name = "";
  submissionForm.proposed_company_main_business = "";
  submissionForm.proposed_company_flagship_products = "";
  submissionForm.proposed_company_domain_area = "";
  submissionForm.proposed_company_application_industries = "";
  submissionForm.proposed_job_title = "";
  submissionForm.proposed_job_responsibilities_text = "";
  submissionForm.proposed_job_technical_projects_text = "";
  submissionForm.proposed_job_required_knowledge_skills_text = "";
  submissionForm.proposed_job_required_tools_platform_text = "";
  submissionForm.employment_type = "signed";
  submissionForm.employment_status = "active";
  submissionForm.start_date = "";
  submissionForm.end_date = "";
  submissionForm.evidence_source = "";
  submissionForm.notes = "";
}

async function submitEmployment() {
  clearFeedback();
  if (!hasBoundStudent.value) {
    errorText.value = "请先绑定学生身份";
    return;
  }
  if (submissionMode.value === "existing") {
    if (!submissionForm.company_id && !submissionForm.job_id) {
      errorText.value = "请选择已有公司，或直接选择岗位";
      return;
    }
  } else if (!submissionForm.proposed_company_name) {
    errorText.value = "新建模式下，公司名称为必填";
    return;
  }
  submissionSaving.value = true;
  try {
    const payload = {
      employment_type: submissionForm.employment_type,
      employment_status: submissionForm.employment_status,
      start_date: submissionForm.start_date || "",
      end_date: submissionForm.end_date || "",
      evidence_source: submissionForm.evidence_source || "",
      notes: submissionForm.notes || "",
      proposed_company_name: submissionMode.value === "new" ? (submissionForm.proposed_company_name || "") : "",
      proposed_company_main_business: submissionMode.value === "new" ? (submissionForm.proposed_company_main_business || "") : "",
      proposed_company_flagship_products: submissionMode.value === "new" ? (submissionForm.proposed_company_flagship_products || "") : "",
      proposed_company_domain_area: submissionMode.value === "new" ? (submissionForm.proposed_company_domain_area || "") : "",
      proposed_company_application_industries: submissionMode.value === "new" ? (submissionForm.proposed_company_application_industries || "") : "",
      proposed_job_title: submissionMode.value === "new" ? (submissionForm.proposed_job_title || "") : "",
      proposed_job_responsibilities_text: submissionMode.value === "new" ? (submissionForm.proposed_job_responsibilities_text || "") : "",
      proposed_job_technical_projects_text: submissionMode.value === "new" ? (submissionForm.proposed_job_technical_projects_text || "") : "",
      proposed_job_required_knowledge_skills_text: submissionMode.value === "new" ? (submissionForm.proposed_job_required_knowledge_skills_text || "") : "",
      proposed_job_required_tools_platform_text: submissionMode.value === "new" ? (submissionForm.proposed_job_required_tools_platform_text || "") : ""
    };
    if (submissionMode.value === "existing" && Number(submissionForm.company_id) > 0) {
      payload.company_id = Number(submissionForm.company_id);
    }
    if (submissionMode.value === "existing" && Number(submissionForm.job_id) > 0) {
      payload.job_id = Number(submissionForm.job_id);
    }
    await api.post("/me/employment-submissions", payload);
    successText.value = "提交成功，等待管理员审核";
    resetSubmissionForm();
    await loadMySubmissions(1);
  } catch (err) {
    errorText.value = pickError(err, "提交就业信息失败");
  } finally {
    submissionSaving.value = false;
  }
}

async function loadMySubmissions(page = 1) {
  clearFeedback();
  mySubmissionLoading.value = true;
  try {
    const params = {
      page,
      page_size: mySubMeta.page_size,
      sort_order: "desc"
    };
    if (mySubmissionStatus.value) {
      params.status = mySubmissionStatus.value;
    }
    const resp = await api.get("/me/employment-submissions", { params });
    mySubmissions.value = resp?.data?.data || [];
    applyMeta(mySubMeta, resp?.data?.meta || {});
  } catch (err) {
    errorText.value = pickError(err, "加载就业提交历史失败");
  } finally {
    mySubmissionLoading.value = false;
  }
}

function clearSelection() {
  selectedIds.value = [];
}

function toggleSelectOne(id, event) {
  const checked = Boolean(event?.target?.checked);
  const value = Number(id);
  const set = new Set(selectedIds.value.map((item) => Number(item)));
  if (checked) {
    set.add(value);
  } else {
    set.delete(value);
  }
  selectedIds.value = Array.from(set);
}

function toggleSelectAll(event) {
  const checked = Boolean(event?.target?.checked);
  const set = new Set(selectedIds.value.map((item) => Number(item)));
  if (checked) {
    pendingIdsInPage.value.forEach((id) => set.add(id));
  } else {
    pendingIdsInPage.value.forEach((id) => set.delete(id));
  }
  selectedIds.value = Array.from(set);
}

async function loadAdminSubmissions(page = 1) {
  clearFeedback();
  adminLoading.value = true;
  try {
    const params = buildAdminQueryParams(page, adminFilters.page_size);
    const resp = await api.get("/admin/employment-submissions", { params });
    adminSubmissions.value = resp?.data?.data || [];
    applyMeta(adminMeta, resp?.data?.meta || {});
    const allowed = new Set(adminSubmissions.value.map((item) => Number(item.id)));
    selectedIds.value = selectedIds.value.filter((id) => allowed.has(Number(id)));
  } catch (err) {
    errorText.value = pickError(err, "加载审核列表失败");
  } finally {
    adminLoading.value = false;
  }
}

async function loadAdminLogs() {
  logLoading.value = true;
  try {
    const resp = await api.get("/system-logs", {
      params: {
        page: 1,
        page_size: 10,
        sort_by: "id",
        sort_order: "desc",
        keyword: "employment-submissions"
      }
    });
    adminLogs.value = resp?.data?.data || [];
  } catch (err) {
    errorText.value = pickError(err, "加载审核日志失败");
  } finally {
    logLoading.value = false;
  }
}

function buildAdminQueryParams(page = 1, pageSize = 20) {
  const params = {
    page,
    page_size: pageSize,
    sort_order: adminFilters.sort_order
  };
  if (adminFilters.status) {
    params.status = adminFilters.status;
  }
  if (adminFilters.keyword) {
    params.keyword = adminFilters.keyword;
  }
  return params;
}

function searchAdminSubmissions() {
  loadAdminSubmissions(1);
}

function resetAdminFilters() {
  adminFilters.status = "pending";
  adminFilters.keyword = "";
  adminFilters.sort_order = "desc";
  loadAdminSubmissions(1);
}

function changeAdminPage(page) {
  if (page < 1 || page > adminMeta.total_pages) {
    return;
  }
  loadAdminSubmissions(page);
}

async function approveOne(item) {
  clearFeedback();
  if (!window.confirm(`确认通过提交 #${item.id} 吗？`)) {
    return;
  }
  const comment = window.prompt("审核备注（可选）", "");
  if (comment === null) {
    return;
  }
  try {
    await api.post(`/admin/employment-submissions/${item.id}/approve`, {
      review_comment: comment
    });
    successText.value = `已通过提交 #${item.id}`;
    await loadAdminSubmissions(adminMeta.page);
    await loadAdminLogs();
  } catch (err) {
    errorText.value = pickError(err, "审批失败");
  }
}

async function rejectOne(item) {
  clearFeedback();
  if (!window.confirm(`确认拒绝提交 #${item.id} 吗？`)) {
    return;
  }
  const comment = window.prompt("请输入拒绝原因", "信息不完整，请补充");
  if (comment === null) {
    return;
  }
  try {
    await api.post(`/admin/employment-submissions/${item.id}/reject`, {
      review_comment: comment || "rejected by admin"
    });
    successText.value = `已拒绝提交 #${item.id}`;
    await loadAdminSubmissions(adminMeta.page);
    await loadAdminLogs();
  } catch (err) {
    errorText.value = pickError(err, "拒绝失败");
  }
}

async function approveSelected() {
  clearFeedback();
  if (!selectedIds.value.length) {
    errorText.value = "请先选择待审核记录";
    return;
  }
  if (!window.confirm(`确认批量通过 ${selectedIds.value.length} 条记录吗？`)) {
    return;
  }
  batchApproving.value = true;
  try {
    const resp = await api.post("/admin/employment-submissions/batch-approve", {
      ids: selectedIds.value,
      review_comment: batchReviewComment.value || ""
    });
    const data = resp?.data?.data || {};
    successText.value = `批量通过完成：成功 ${data.approved_count || 0} 条，跳过 ${data.skipped_count || 0} 条`;
    clearSelection();
    await loadAdminSubmissions(adminMeta.page);
    await loadAdminLogs();
  } catch (err) {
    errorText.value = pickError(err, "批量审批失败");
  } finally {
    batchApproving.value = false;
  }
}

async function rejectSelected() {
  clearFeedback();
  if (!selectedIds.value.length) {
    errorText.value = "请先选择待审核记录";
    return;
  }
  if (!window.confirm(`确认批量拒绝 ${selectedIds.value.length} 条记录吗？`)) {
    return;
  }
  const comment = window.prompt("批量拒绝原因", "信息不完整，请补充");
  if (comment === null) {
    return;
  }
  batchRejecting.value = true;
  try {
    const resp = await api.post("/admin/employment-submissions/batch-reject", {
      ids: selectedIds.value,
      review_comment: comment || "rejected by admin"
    });
    const data = resp?.data?.data || {};
    successText.value = `批量拒绝完成：成功 ${data.rejected_count || 0} 条，跳过 ${data.skipped_count || 0} 条`;
    clearSelection();
    await loadAdminSubmissions(adminMeta.page);
    await loadAdminLogs();
  } catch (err) {
    errorText.value = pickError(err, "批量拒绝失败");
  } finally {
    batchRejecting.value = false;
  }
}

async function exportAdminSubmissions() {
  clearFeedback();
  exporting.value = true;
  try {
    const resp = await api.get("/admin/employment-submissions/export", {
      params: buildAdminQueryParams(1, 10000),
      responseType: "blob"
    });
    const blob = new Blob([resp.data], { type: "text/csv;charset=utf-8" });
    const url = window.URL.createObjectURL(blob);
    const link = document.createElement("a");
    const now = new Date();
    const ts = `${now.getFullYear()}${String(now.getMonth() + 1).padStart(2, "0")}${String(now.getDate()).padStart(2, "0")}_${String(now.getHours()).padStart(2, "0")}${String(now.getMinutes()).padStart(2, "0")}${String(now.getSeconds()).padStart(2, "0")}`;
    link.href = url;
    link.download = `employment_submissions_${ts}.csv`;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    window.URL.revokeObjectURL(url);
    successText.value = "已导出审核列表";
  } catch (err) {
    errorText.value = pickError(err, "导出失败");
  } finally {
    exporting.value = false;
  }
}

onMounted(async () => {
  if (isAdmin.value) {
    await Promise.all([loadAdminSubmissions(1), loadAdminLogs()]);
    return;
  }

  try {
    await loadStaticOptions();
    await loadMyProfile();
    if (hasBoundStudent.value) {
      await loadMySubmissions(1);
    }
  } catch (err) {
    errorText.value = pickError(err, "初始化个人主页失败");
  }
});
</script>

<style scoped>
.page {
  display: grid;
  gap: 12px;
}

h2 {
  margin: 0;
}

.desc {
  margin: 0;
  color: #475467;
}

.panel {
  background: #fff;
  border-radius: 10px;
  padding: 14px;
  box-shadow: 0 6px 16px rgba(16, 24, 40, 0.08);
}

.panel-summary {
  cursor: pointer;
  font-weight: 600;
}

.panel-body {
  margin-top: 10px;
  display: grid;
  gap: 12px;
}

.form-panel {
  padding: 12px 14px;
}

.grid {
  display: grid;
  gap: 10px;
  grid-template-columns: repeat(4, minmax(0, 1fr));
}

.compact-grid {
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

label {
  display: grid;
  gap: 6px;
  font-size: 13px;
  color: #344054;
}

input,
select,
textarea {
  border: 1px solid #d0d5dd;
  border-radius: 8px;
  padding: 8px 10px;
  font-size: 14px;
  font-family: inherit;
}

input:disabled {
  background: #f2f4f7;
}

.actions {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

button {
  border: none;
  border-radius: 8px;
  background: #0d2b45;
  color: #fff;
  padding: 8px 12px;
  cursor: pointer;
}

button.secondary {
  background: #4267b2;
}

button.danger {
  background: #b42318;
}

button.small {
  font-size: 12px;
  padding: 4px 8px;
}

button:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

th,
td {
  border-bottom: 1px solid #eaecf0;
  padding: 8px;
  text-align: left;
  vertical-align: top;
}

.row-actions {
  display: flex;
  gap: 6px;
  flex-wrap: wrap;
}

.meta {
  margin: 0;
  color: #475467;
  font-size: 13px;
}

.meta-tag {
  display: inline-block;
  margin-left: 4px;
  padding: 1px 6px;
  border-radius: 10px;
  background: #e9f2ff;
  color: #1e4f8f;
  font-size: 11px;
}

.error {
  margin: 0;
  color: #b42318;
}

.success {
  margin: 0;
  color: #027a48;
}

@media (max-width: 1200px) {
  .grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 760px) {
  .grid,
  .compact-grid {
    grid-template-columns: 1fr;
  }
}
</style>
