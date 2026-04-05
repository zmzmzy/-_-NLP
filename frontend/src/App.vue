<template>
  <div class="layout">
    <div class="layout-bg" aria-hidden="true">
      <div class="aurora"></div>
      <div class="orb orb-a"></div>
      <div class="orb orb-b"></div>
      <div class="orb orb-c"></div>
      <div class="orb orb-d"></div>
      <div class="grid-overlay"></div>
    </div>

    <header v-if="showTopbar" class="topbar">
      <nav class="nav">
        <template v-if="!isViewer">
          <router-link class="nav-link" to="/major-center">专业中心</router-link>
          <router-link class="nav-link" to="/job-center">岗位中心</router-link>
          <router-link class="nav-link" to="/match">匹配分析</router-link>
          <router-link class="nav-link" to="/dashboard">决策看板</router-link>
        </template>
        <router-link class="nav-link" to="/profile">{{ isAdmin ? "审核中心" : "个人主页" }}</router-link>
      </nav>
      <div class="user-box">
        <span class="username">用户：{{ username }}（{{ roleLabel }}）</span>
        <button class="logout-btn" :disabled="logoutLoading" @click="handleLogout">
          {{ logoutLoading ? "退出中..." : "退出登录" }}
        </button>
      </div>
    </header>
    <main class="content" :class="{ 'content-auth': !showTopbar }">
      <router-view />
    </main>
  </div>
</template>

<script setup>
import { computed, ref } from "vue";
import { useRoute, useRouter } from "vue-router";
import { useAuthStore } from "./stores/auth";

const route = useRoute();
const router = useRouter();
const authStore = useAuthStore();

const logoutLoading = ref(false);
const showTopbar = computed(() => !["login", "register", "password"].includes(String(route.name || "")));
const username = computed(() => authStore.user?.username || "未登录");
const role = computed(() => authStore.user?.role || "viewer");
const isViewer = computed(() => role.value === "viewer");
const isAdmin = computed(() => role.value === "admin");
const roleLabel = computed(() => {
  if (role.value === "admin") {
    return "管理员";
  }
  if (role.value === "teacher") {
    return "教师";
  }
  return "普通用户";
});

async function handleLogout() {
  logoutLoading.value = true;
  try {
    await authStore.logout();
  } finally {
    logoutLoading.value = false;
    router.replace("/login");
  }
}
</script>

<style scoped>
.layout {
  position: relative;
  overflow: hidden;
  min-height: 100vh;
  font-family: "Space Grotesk", "PingFang SC", "Microsoft YaHei", sans-serif;
}

.layout-bg {
  position: fixed;
  inset: 0;
  z-index: 0;
  pointer-events: none;
}

.aurora {
  position: absolute;
  inset: 0;
  background:
    radial-gradient(circle at 8% 12%, rgba(59, 130, 246, 0.26), transparent 33%),
    radial-gradient(circle at 84% 18%, rgba(45, 212, 191, 0.2), transparent 30%),
    radial-gradient(circle at 16% 86%, rgba(125, 211, 252, 0.2), transparent 32%),
    linear-gradient(140deg, #060f21 0%, #0c1c34 36%, #0d2940 100%);
}

.orb {
  position: absolute;
  border-radius: 9999px;
  background: rgba(255, 255, 255, 0.12);
  filter: blur(56px);
  animation: float 7s ease-in-out infinite;
}

.orb-a {
  top: 10%;
  left: 8%;
  width: 180px;
  height: 180px;
}

.orb-b {
  top: 20%;
  right: 10%;
  width: 240px;
  height: 240px;
  animation-delay: 0.8s;
}

.orb-c {
  bottom: 12%;
  left: 18%;
  width: 220px;
  height: 220px;
  animation-delay: 1.6s;
}

.orb-d {
  right: 18%;
  bottom: 10%;
  width: 160px;
  height: 160px;
  animation-delay: 2.3s;
}

.grid-overlay {
  position: absolute;
  inset: 0;
  opacity: 0.18;
  background:
    linear-gradient(rgba(255, 255, 255, 0.08) 1px, transparent 1px),
    linear-gradient(90deg, rgba(255, 255, 255, 0.08) 1px, transparent 1px);
  background-size: 42px 42px;
}

.topbar {
  position: sticky;
  top: 14px;
  z-index: 20;
  margin: 14px auto 0;
  width: min(1320px, calc(100% - 28px));
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  padding: 14px 20px;
  border: 1px solid rgba(255, 255, 255, 0.16);
  border-radius: 22px;
  background: rgba(255, 255, 255, 0.09);
  backdrop-filter: blur(16px);
  color: #fff;
  box-shadow: 0 14px 36px rgba(3, 8, 26, 0.35);
}

.nav {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
}

.nav-link {
  color: #fff;
  text-decoration: none;
  padding: 8px 12px;
  border-radius: 9999px;
  border: 1px solid rgba(255, 255, 255, 0.16);
  background: rgba(255, 255, 255, 0.06);
  transition: all 0.24s ease;
}

.nav-link:hover {
  transform: translateY(-1px);
  background: rgba(255, 255, 255, 0.14);
}

.nav-link.router-link-active {
  background: linear-gradient(120deg, rgba(37, 99, 235, 0.84), rgba(14, 165, 233, 0.78));
  border-color: rgba(255, 255, 255, 0.24);
  box-shadow: 0 10px 24px rgba(14, 116, 144, 0.38);
}

.user-box {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}

.username {
  font-size: 13px;
  opacity: 0.9;
}

.logout-btn {
  border: 1px solid rgba(255, 255, 255, 0.22);
  background: rgba(255, 255, 255, 0.08);
  color: #fff;
  border-radius: 9999px;
  padding: 8px 14px;
  cursor: pointer;
  transition: all 0.22s ease;
}

.logout-btn:hover {
  transform: translateY(-1px);
  background: rgba(255, 255, 255, 0.15);
}

.logout-btn:disabled {
  opacity: 0.65;
  cursor: not-allowed;
}

.content {
  position: relative;
  z-index: 10;
  width: min(1320px, calc(100% - 28px));
  margin: 16px auto 0;
  padding-bottom: 18px;
}

.content-auth {
  width: 100%;
  margin-top: 0;
}

@media (max-width: 900px) {
  .topbar {
    flex-direction: column;
    align-items: flex-start;
    top: 10px;
    margin-top: 10px;
    border-radius: 18px;
    padding: 12px 14px;
  }

  .content {
    width: calc(100% - 16px);
    margin-top: 10px;
  }
}

@keyframes float {
  0% {
    transform: translate3d(0, 0, 0) scale(1);
  }
  50% {
    transform: translate3d(12px, -18px, 0) scale(1.06);
  }
  100% {
    transform: translate3d(0, 0, 0) scale(1);
  }
}
</style>
