<template>
  <section class="page gm-page jc-shell fade-up">
    <aside class="panel jc-sidebar">
      <div class="jc-brand">
        <p class="jc-tag">岗位中心</p>
        <h3>Job Center</h3>
        <p class="hint">企业、岗位、导入统一管理</p>
      </div>

      <nav class="jc-nav">
        <router-link
          v-for="item in navItems"
          :key="item.name"
          class="jc-nav-link"
          :class="{ active: route.name === item.name }"
          :to="item.to"
        >
          <span class="jc-index">{{ item.index }}</span>
          <span class="jc-label-wrap">
            <span class="jc-label">{{ item.label }}</span>
            <span class="jc-label-en">{{ item.en }}</span>
          </span>
        </router-link>
      </nav>

      <div class="jc-tip">
        <p>结构建议</p>
        <p class="hint">总览 + 实体列表 + 导入中心，降低页面耦合度。</p>
      </div>
    </aside>

    <div class="jc-content">
      <header class="panel jc-header">
        <p class="jc-crumb">业务中心 / {{ currentMeta.label }}</p>
        <div class="jc-head-row">
          <div>
            <h2>{{ currentMeta.label }}</h2>
            <p class="desc">{{ currentMeta.desc }}</p>
          </div>
          <div class="actions">
            <router-link class="jc-link-btn secondary" :to="{ name: 'job-center-imports' }">导入中心</router-link>
            <router-link class="jc-link-btn" :to="{ name: 'job-center-overview' }">返回总览</router-link>
          </div>
        </div>
      </header>

      <router-view />
    </div>
  </section>
</template>

<script setup>
import { computed } from "vue";
import { useRoute } from "vue-router";

const route = useRoute();

const navItems = [
  { index: "01", name: "job-center-overview", label: "总览", en: "Overview", to: { name: "job-center-overview" } },
  { index: "02", name: "job-center-companies", label: "企业管理", en: "Companies", to: { name: "job-center-companies" } },
  { index: "03", name: "job-center-jobs", label: "岗位管理", en: "Jobs", to: { name: "job-center-jobs" } },
  { index: "04", name: "job-center-imports", label: "导入中心", en: "Imports", to: { name: "job-center-imports" } }
];

const metaMap = {
  "job-center-overview": {
    label: "岗位中心总览",
    desc: "展示企业和岗位的关键统计，快速进入核心操作页面。"
  },
  "job-center-companies": {
    label: "企业管理",
    desc: "维护企业主体信息，支持按领域和地域维度筛选。"
  },
  "job-center-jobs": {
    label: "岗位管理",
    desc: "维护岗位文本要求，支持按企业、学历、岗位类型检索。"
  },
  "job-center-imports": {
    label: "导入中心",
    desc: "统一岗位导入预检、应用与批次历史查询。"
  }
};

const currentMeta = computed(() => metaMap[String(route.name || "")] || metaMap["job-center-overview"]);
</script>

<style scoped>
.jc-shell {
  margin: 0;
  padding: 14px;
  display: grid;
  gap: 14px;
  grid-template-columns: 250px 1fr;
  min-height: calc(100vh - 120px);
}

.jc-sidebar {
  padding: 14px;
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.jc-brand h3 {
  margin: 6px 0;
  font-size: 20px;
}

.jc-tag {
  margin: 0;
  display: inline-flex;
  align-items: center;
  border: 1px solid rgba(173, 211, 255, 0.35);
  border-radius: 999px;
  padding: 2px 10px;
  font-size: 12px;
  color: #d2e8ff;
  background: rgba(147, 197, 253, 0.14);
}

.jc-nav {
  display: grid;
  gap: 8px;
}

.jc-nav-link {
  text-decoration: none;
  border: 1px solid rgba(173, 211, 255, 0.24);
  border-radius: 14px;
  padding: 8px 10px;
  color: #cfe3fa;
  display: flex;
  align-items: center;
  gap: 10px;
  transition: all 0.2s ease;
}

.jc-nav-link:hover {
  transform: translateY(-1px);
  background: rgba(255, 255, 255, 0.08);
}

.jc-nav-link.active {
  background: linear-gradient(120deg, rgba(37, 99, 235, 0.55), rgba(20, 184, 166, 0.46));
  border-color: rgba(186, 224, 255, 0.45);
  box-shadow: 0 10px 20px rgba(14, 116, 144, 0.24);
}

.jc-index {
  width: 30px;
  height: 30px;
  border-radius: 10px;
  display: grid;
  place-items: center;
  font-size: 12px;
  color: #e6f3ff;
  background: rgba(255, 255, 255, 0.1);
}

.jc-label-wrap {
  display: grid;
}

.jc-label {
  font-size: 14px;
}

.jc-label-en {
  font-size: 11px;
  opacity: 0.72;
}

.jc-tip {
  margin-top: auto;
  border: 1px solid rgba(173, 211, 255, 0.2);
  border-radius: 14px;
  padding: 10px;
  background: rgba(5, 18, 35, 0.28);
}

.jc-tip p {
  margin: 0;
}

.jc-content {
  display: grid;
  gap: 12px;
  min-width: 0;
}

.jc-header {
  padding: 14px;
}

.jc-crumb {
  margin: 0;
  font-size: 12px;
  color: #a6bdd8;
}

.jc-head-row {
  margin-top: 8px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.jc-head-row h2 {
  margin: 0;
}

.jc-head-row .desc {
  margin-top: 6px;
}

.jc-link-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 12px;
  border: 1px solid rgba(125, 211, 252, 0.44);
  background: linear-gradient(120deg, #2563eb, #0ea5e9 55%, #14b8a6);
  color: #f8fbff;
  text-decoration: none;
  padding: 8px 12px;
  transition: all 0.2s ease;
}

.jc-link-btn.secondary {
  border-color: rgba(147, 197, 253, 0.34);
  background: rgba(255, 255, 255, 0.11);
}

.jc-link-btn:hover {
  transform: translateY(-1px);
}

@media (max-width: 1000px) {
  .jc-shell {
    grid-template-columns: 1fr;
  }

  .jc-nav {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 700px) {
  .jc-nav {
    grid-template-columns: 1fr;
  }

  .jc-head-row {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
