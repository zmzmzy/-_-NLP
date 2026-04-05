<template>
  <section class="page gm-page mtc-shell fade-up">
    <aside class="panel mtc-sidebar">
      <div class="mtc-brand">
        <p class="mtc-tag">匹配分析</p>
        <h3>Matching Analysis</h3>
        <p class="hint">四类匹配任务 + 历史检索</p>
      </div>

      <nav class="mtc-nav">
        <router-link
          v-for="item in navItems"
          :key="item.name"
          class="mtc-nav-link"
          :class="{ active: route.name === item.name }"
          :to="item.to"
        >
          <span class="mtc-index">{{ item.index }}</span>
          <span class="mtc-label-wrap">
            <span class="mtc-label">{{ item.label }}</span>
            <span class="mtc-label-en">{{ item.en }}</span>
          </span>
        </router-link>
      </nav>

      <div class="mtc-tip">
        <p>结构建议</p>
        <p class="hint">按任务流拆页：单条、多岗位、批量、历史，避免一个超级页面堆叠全部表单。</p>
      </div>
    </aside>

    <div class="mtc-content">
      <header class="panel mtc-header">
        <p class="mtc-crumb">业务中心 / 匹配分析 / {{ currentMeta.label }}</p>
        <div class="mtc-head-row">
          <div>
            <h2>{{ currentMeta.label }}</h2>
            <p class="desc">{{ currentMeta.desc }}</p>
          </div>
          <div class="actions">
            <router-link class="mtc-link-btn secondary" :to="{ name: 'match-center-history' }">历史记录</router-link>
            <router-link class="mtc-link-btn" :to="{ name: 'match-center-overview' }">返回总览</router-link>
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
  { index: "01", name: "match-center-overview", label: "分析总览", en: "Overview", to: { name: "match-center-overview" } },
  { index: "02", name: "match-center-single", label: "单条匹配", en: "Single", to: { name: "match-center-single" } },
  { index: "03", name: "match-center-multi", label: "多岗位匹配", en: "Multi", to: { name: "match-center-multi" } },
  { index: "04", name: "match-center-batch", label: "全量级匹配", en: "Batch", to: { name: "match-center-batch" } },
  { index: "05", name: "match-center-history", label: "历史记录", en: "History", to: { name: "match-center-history" } }
];

const metaMap = {
  "match-center-overview": {
    label: "分析总览",
    desc: "查看算法入口、任务状态和历史摘要，快速进入对应任务流。"
  },
  "match-center-single": {
    label: "单条匹配",
    desc: "单个专业与单个岗位的即时匹配计算，重点展示可解释结果。"
  },
  "match-center-multi": {
    label: "多岗位匹配",
    desc: "单专业对岗位集合进行批量计算，输出排序与异常清单。"
  },
  "match-center-batch": {
    label: "全量级匹配",
    desc: "支持专业级、学院级和全量级重算任务，集中查看执行结果。"
  },
  "match-center-history": {
    label: "历史记录",
    desc: "按专业、岗位、算法、模式检索历史计算结果并做清理。"
  }
};

const currentMeta = computed(() => metaMap[String(route.name || "")] || metaMap["match-center-overview"]);
</script>

<style scoped>
.mtc-shell {
  margin: 0;
  padding: 14px;
  display: grid;
  gap: 14px;
  grid-template-columns: 260px 1fr;
  min-height: calc(100vh - 120px);
}

.mtc-sidebar {
  padding: 14px;
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.mtc-brand h3 {
  margin: 6px 0;
  font-size: 20px;
}

.mtc-tag {
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

.mtc-nav {
  display: grid;
  gap: 8px;
}

.mtc-nav-link {
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

.mtc-nav-link:hover {
  transform: translateY(-1px);
  background: rgba(255, 255, 255, 0.08);
}

.mtc-nav-link.active {
  background: linear-gradient(120deg, rgba(37, 99, 235, 0.55), rgba(20, 184, 166, 0.46));
  border-color: rgba(186, 224, 255, 0.45);
  box-shadow: 0 10px 20px rgba(14, 116, 144, 0.24);
}

.mtc-index {
  width: 30px;
  height: 30px;
  border-radius: 10px;
  display: grid;
  place-items: center;
  font-size: 12px;
  color: #e6f3ff;
  background: rgba(255, 255, 255, 0.1);
}

.mtc-label-wrap {
  display: grid;
}

.mtc-label {
  font-size: 14px;
}

.mtc-label-en {
  font-size: 11px;
  opacity: 0.72;
}

.mtc-tip {
  margin-top: auto;
  border: 1px solid rgba(173, 211, 255, 0.2);
  border-radius: 14px;
  padding: 10px;
  background: rgba(5, 18, 35, 0.28);
}

.mtc-tip p {
  margin: 0;
}

.mtc-content {
  display: grid;
  gap: 12px;
  min-width: 0;
}

.mtc-header {
  padding: 14px;
}

.mtc-crumb {
  margin: 0;
  font-size: 12px;
  color: #a6bdd8;
}

.mtc-head-row {
  margin-top: 8px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.mtc-head-row h2 {
  margin: 0;
}

.mtc-head-row .desc {
  margin-top: 6px;
}

.mtc-link-btn {
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

.mtc-link-btn.secondary {
  border-color: rgba(147, 197, 253, 0.34);
  background: rgba(255, 255, 255, 0.11);
}

.mtc-link-btn:hover {
  transform: translateY(-1px);
}

@media (max-width: 1000px) {
  .mtc-shell {
    grid-template-columns: 1fr;
  }

  .mtc-nav {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 700px) {
  .mtc-nav {
    grid-template-columns: 1fr;
  }

  .mtc-head-row {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
