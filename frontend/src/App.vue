<template>
  <div class="layout">
    <header v-if="showTopbar" class="topbar">
      <div class="brand">
        <h1>高校专业与社会岗位匹配系统 v0.4</h1>
        <p>毕业设计演示版</p>
      </div>
      <div class="right-area">
        <nav>
          <template v-if="!isViewer">
            <router-link to="/majors">专业中心</router-link>
            <router-link to="/jobs">岗位中心</router-link>
            <router-link to="/match">匹配分析</router-link>
            <router-link to="/dashboard">决策看板</router-link>
          </template>
          <router-link to="/profile">{{ isAdmin ? "审核中心" : "个人主页" }}</router-link>
        </nav>
        <div class="user-box">
          <span class="username">用户：{{ username }}（{{ roleLabel }}）</span>
          <button :disabled="logoutLoading" @click="handleLogout">
            {{ logoutLoading ? "退出中..." : "退出登录" }}
          </button>
        </div>
      </div>
    </header>
    <main class="content">
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
  font-family: "PingFang SC", "Microsoft YaHei", Arial, sans-serif;
  min-height: 100vh;
  background: #f4f6f9;
}

.topbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 12px 20px;
  background: #0d2b45;
  color: #fff;
}

.brand h1 {
  margin: 0;
  font-size: 20px;
}

.brand p {
  margin: 2px 0 0;
  font-size: 12px;
  opacity: 0.85;
}

nav {
  display: flex;
  gap: 12px;
}

.right-area {
  display: flex;
  align-items: center;
  gap: 16px;
}

a {
  color: #fff;
  text-decoration: none;
  padding: 6px 10px;
  border-radius: 6px;
}

a.router-link-active {
  background: #1c5d99;
}

.user-box {
  display: flex;
  align-items: center;
  gap: 8px;
}

.username {
  font-size: 13px;
  opacity: 0.95;
}

button {
  border: 1px solid #5f86aa;
  background: transparent;
  color: #fff;
  border-radius: 6px;
  padding: 6px 10px;
  cursor: pointer;
}

button:disabled {
  opacity: 0.65;
  cursor: not-allowed;
}

.content {
  padding: 20px;
}

@media (max-width: 900px) {
  .topbar {
    flex-direction: column;
    align-items: flex-start;
  }

  .right-area {
    width: 100%;
    flex-direction: column;
    align-items: flex-start;
    gap: 10px;
  }

  nav {
    flex-wrap: wrap;
  }
}
</style>
