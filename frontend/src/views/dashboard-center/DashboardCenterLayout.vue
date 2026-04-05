<template>
  <section class="page gm-page dbc-shell fade-up">
    <aside class="panel dbc-sidebar">
      <div class="dbc-brand">
        <p class="dbc-tag">决策看板</p>
        <h3>Decision Board</h3>
        <p class="hint">排名、缺口、解读、历史拆分管理</p>
      </div>

      <nav class="dbc-nav">
        <router-link
          v-for="item in navItems"
          :key="item.name"
          class="dbc-nav-link"
          :class="{ active: route.name === item.name }"
          :to="item.to"
        >
          <span class="dbc-index">{{ item.index }}</span>
          <span class="dbc-label-wrap">
            <span class="dbc-label">{{ item.label }}</span>
            <span class="dbc-label-en">{{ item.en }}</span>
          </span>
        </router-link>
      </nav>

      <div class="dbc-tip">
        <p>结构建议</p>
        <p class="hint">保持单页单任务：筛选、排名、缺口、AI 解读、记录独立维护。</p>
      </div>
    </aside>

    <div class="dbc-content">
      <header class="panel dbc-header">
        <p class="dbc-crumb">业务中心 / 决策看板 / {{ currentMeta.label }}</p>
        <div class="dbc-head-row">
          <div>
            <h2>{{ currentMeta.label }}</h2>
            <p class="desc">{{ currentMeta.desc }}</p>
          </div>
          <div class="actions">
            <router-link class="dbc-link-btn secondary" :to="{ name: 'dashboard-legacy' }">旧版看板</router-link>
            <router-link class="dbc-link-btn" :to="{ name: 'dashboard-center-overview' }">返回总览</router-link>
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
  { index: "01", name: "dashboard-center-overview", label: "分析总览", en: "Overview", to: { name: "dashboard-center-overview" } },
  { index: "02", name: "dashboard-center-ranking", label: "专业排名", en: "Ranking", to: { name: "dashboard-center-ranking" } },
  { index: "03", name: "dashboard-center-gaps", label: "技能缺口", en: "Gaps", to: { name: "dashboard-center-gaps" } },
  { index: "04", name: "dashboard-center-insight", label: "AI 解读", en: "Insight", to: { name: "dashboard-center-insight" } },
  { index: "05", name: "dashboard-center-history", label: "看板记录", en: "History", to: { name: "dashboard-center-history" } }
];

const metaMap = {
  "dashboard-center-overview": {
    label: "分析总览",
    desc: "展示核心指标、快捷入口和风险提示，快速定位下一步业务动作。"
  },
  "dashboard-center-ranking": {
    label: "专业排名",
    desc: "按对口率、就业率和综合分进行排名筛选，支持下钻至专业详情。"
  },
  "dashboard-center-gaps": {
    label: "技能缺口",
    desc: "查看各专业技能缺口 TopN，识别课程建设和训练方向。"
  },
  "dashboard-center-insight": {
    label: "AI 解读",
    desc: "基于后端 Agent 中间件生成结构化解读，支持模型约束、RAG 与历史追踪。"
  },
  "dashboard-center-history": {
    label: "看板记录",
    desc: "记录看板操作与解读结果，便于复盘与复用参数。"
  }
};

const currentMeta = computed(() => metaMap[String(route.name || "")] || metaMap["dashboard-center-overview"]);
</script>

<style scoped>
.dbc-shell {
  margin: 0;
  padding: 14px;
  display: grid;
  gap: 14px;
  grid-template-columns: 260px 1fr;
  min-height: calc(100vh - 120px);
}

.dbc-sidebar {
  padding: 14px;
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.dbc-brand h3 {
  margin: 6px 0;
  font-size: 20px;
}

.dbc-tag {
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

.dbc-nav {
  display: grid;
  gap: 8px;
}

.dbc-nav-link {
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

.dbc-nav-link:hover {
  transform: translateY(-1px);
  background: rgba(255, 255, 255, 0.08);
}

.dbc-nav-link.active {
  background: linear-gradient(120deg, rgba(37, 99, 235, 0.55), rgba(20, 184, 166, 0.46));
  border-color: rgba(186, 224, 255, 0.45);
  box-shadow: 0 10px 20px rgba(14, 116, 144, 0.24);
}

.dbc-index {
  width: 30px;
  height: 30px;
  border-radius: 10px;
  display: grid;
  place-items: center;
  font-size: 12px;
  color: #e6f3ff;
  background: rgba(255, 255, 255, 0.1);
}

.dbc-label-wrap {
  display: grid;
}

.dbc-label {
  font-size: 14px;
}

.dbc-label-en {
  font-size: 11px;
  opacity: 0.72;
}

.dbc-tip {
  margin-top: auto;
  border: 1px solid rgba(173, 211, 255, 0.2);
  border-radius: 14px;
  padding: 10px;
  background: rgba(5, 18, 35, 0.28);
}

.dbc-tip p {
  margin: 0;
}

.dbc-content {
  display: grid;
  gap: 12px;
  min-width: 0;
}

.dbc-header {
  padding: 14px;
}

.dbc-crumb {
  margin: 0;
  font-size: 12px;
  color: #a6bdd8;
}

.dbc-head-row {
  margin-top: 8px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.dbc-head-row h2 {
  margin: 0;
}

.dbc-head-row .desc {
  margin-top: 6px;
}

.dbc-link-btn {
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

.dbc-link-btn.secondary {
  border-color: rgba(147, 197, 253, 0.34);
  background: rgba(255, 255, 255, 0.11);
}

.dbc-link-btn:hover {
  transform: translateY(-1px);
}

@media (max-width: 1000px) {
  .dbc-shell {
    grid-template-columns: 1fr;
  }

  .dbc-nav {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 700px) {
  .dbc-nav {
    grid-template-columns: 1fr;
  }

  .dbc-head-row {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
