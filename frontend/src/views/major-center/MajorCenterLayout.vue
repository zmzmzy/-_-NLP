<template>
  <section class="page gm-page mc-shell fade-up">
    <aside class="panel mc-sidebar">
      <div class="mc-brand">
        <p class="mc-tag">专业中心</p>
        <h3>Professional Center</h3>
        <p class="hint">学院、专业、学生、导入统一管理</p>
      </div>

      <nav class="mc-nav">
        <router-link
          v-for="item in navItems"
          :key="item.name"
          class="mc-nav-link"
          :class="{ active: route.name === item.name }"
          :to="item.to"
        >
          <span class="mc-index">{{ item.index }}</span>
          <span class="mc-label-wrap">
            <span class="mc-label">{{ item.label }}</span>
            <span class="mc-label-en">{{ item.en }}</span>
          </span>
        </router-link>
      </nav>

      <div class="mc-tip">
        <p>结构建议</p>
        <p class="hint">总览 + 列表 + 详情 + 导入中心，避免单页堆砌。</p>
      </div>
    </aside>

    <div class="mc-content">
      <header class="panel mc-header">
        <p class="mc-crumb">业务中心 / {{ currentMeta.label }}</p>
        <div class="mc-head-row">
          <div>
            <h2>{{ currentMeta.label }}</h2>
            <p class="desc">{{ currentMeta.desc }}</p>
          </div>
          <div class="actions">
            <router-link class="mc-link-btn secondary" :to="{ name: 'major-center-imports' }">导入中心</router-link>
            <router-link class="mc-link-btn" :to="{ name: 'major-center-overview' }">返回总览</router-link>
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
  { index: "01", name: "major-center-overview", label: "总览", en: "Overview", to: { name: "major-center-overview" } },
  { index: "02", name: "major-center-colleges", label: "学院管理", en: "Colleges", to: { name: "major-center-colleges" } },
  { index: "03", name: "major-center-majors", label: "专业管理", en: "Majors", to: { name: "major-center-majors" } },
  { index: "04", name: "major-center-students", label: "学生管理", en: "Students", to: { name: "major-center-students" } },
  { index: "05", name: "major-center-imports", label: "导入中心", en: "Imports", to: { name: "major-center-imports" } }
];

const metaMap = {
  "major-center-overview": {
    label: "专业中心总览",
    desc: "通过关键统计、快捷入口和最近导入动态，快速掌握当前业务状态。"
  },
  "major-center-colleges": {
    label: "学院管理",
    desc: "维护学院基础信息，并从学院维度进入下属专业管理。"
  },
  "major-center-majors": {
    label: "专业管理",
    desc: "维护专业信息与画像内容，并支持按学院过滤和详情下钻。"
  },
  "major-center-students": {
    label: "学生管理",
    desc: "维护学生信息并支持按学院、专业、毕业年份等条件筛选。"
  },
  "major-center-imports": {
    label: "导入中心",
    desc: "统一模板下载、预检导入、确认应用与批次历史管理。"
  }
};

const currentMeta = computed(() => metaMap[String(route.name || "")] || metaMap["major-center-overview"]);
</script>

<style scoped>
.mc-shell {
  margin: 0;
  padding: 14px;
  display: grid;
  gap: 14px;
  grid-template-columns: 260px 1fr;
  min-height: calc(100vh - 120px);
}

.mc-sidebar {
  padding: 14px;
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.mc-brand h3 {
  margin: 6px 0;
  font-size: 20px;
}

.mc-tag {
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

.mc-nav {
  display: grid;
  gap: 8px;
}

.mc-nav-link {
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

.mc-nav-link:hover {
  transform: translateY(-1px);
  background: rgba(255, 255, 255, 0.08);
}

.mc-nav-link.active {
  background: linear-gradient(120deg, rgba(37, 99, 235, 0.55), rgba(20, 184, 166, 0.46));
  border-color: rgba(186, 224, 255, 0.45);
  box-shadow: 0 10px 20px rgba(14, 116, 144, 0.24);
}

.mc-index {
  width: 30px;
  height: 30px;
  border-radius: 10px;
  display: grid;
  place-items: center;
  font-size: 12px;
  color: #e6f3ff;
  background: rgba(255, 255, 255, 0.1);
}

.mc-label-wrap {
  display: grid;
}

.mc-label {
  font-size: 14px;
}

.mc-label-en {
  font-size: 11px;
  opacity: 0.72;
}

.mc-tip {
  margin-top: auto;
  border: 1px solid rgba(173, 211, 255, 0.2);
  border-radius: 14px;
  padding: 10px;
  background: rgba(5, 18, 35, 0.28);
}

.mc-tip p {
  margin: 0;
}

.mc-content {
  display: grid;
  gap: 12px;
  min-width: 0;
}

.mc-header {
  padding: 14px;
}

.mc-crumb {
  margin: 0;
  font-size: 12px;
  color: #a6bdd8;
}

.mc-head-row {
  margin-top: 8px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.mc-head-row h2 {
  margin: 0;
}

.mc-head-row .desc {
  margin-top: 6px;
}

.mc-link-btn {
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

.mc-link-btn.secondary {
  border-color: rgba(147, 197, 253, 0.34);
  background: rgba(255, 255, 255, 0.11);
}

.mc-link-btn:hover {
  transform: translateY(-1px);
}

@media (max-width: 1000px) {
  .mc-shell {
    grid-template-columns: 1fr;
  }

  .mc-sidebar {
    gap: 10px;
  }

  .mc-nav {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 700px) {
  .mc-nav {
    grid-template-columns: 1fr;
  }

  .mc-head-row {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
