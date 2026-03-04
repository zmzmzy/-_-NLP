<template>
  <section class="login-page">
    <div class="login-card">
      <h2>系统登录</h2>
      <p class="hint">请输入管理员账号，进入毕业设计系统。</p>

      <form @submit.prevent="submitLogin">
        <label>
          用户名
          <input v-model.trim="form.username" type="text" placeholder="请输入用户名" />
        </label>

        <label>
          密码
          <input v-model="form.password" type="password" placeholder="请输入密码" />
        </label>

        <p v-if="errorText" class="error">{{ errorText }}</p>

        <button :disabled="loading" type="submit">
          {{ loading ? "登录中..." : "登录" }}
        </button>
      </form>
    </div>
  </section>
</template>

<script setup>
import { reactive, ref } from "vue";
import { useRoute, useRouter } from "vue-router";
import { useAuthStore } from "../stores/auth";

const router = useRouter();
const route = useRoute();
const authStore = useAuthStore();

const form = reactive({
  username: "admin",
  password: "admin123"
});

const loading = ref(false);
const errorText = ref("");

async function submitLogin() {
  errorText.value = "";
  if (!form.username || !form.password) {
    errorText.value = "用户名和密码不能为空";
    return;
  }

  loading.value = true;
  try {
    await authStore.login(form.username, form.password);
    const redirect = typeof route.query.redirect === "string" ? route.query.redirect : "/";
    router.replace(redirect);
  } catch (err) {
    errorText.value = err?.response?.data?.error || "登录失败，请检查账号密码";
  } finally {
    loading.value = false;
  }
}
</script>

<style scoped>
.login-page {
  min-height: calc(100vh - 40px);
  display: grid;
  place-items: center;
}

.login-card {
  width: min(420px, 92vw);
  background: #fff;
  border-radius: 12px;
  padding: 22px;
  box-shadow: 0 10px 28px rgba(13, 43, 69, 0.12);
}

h2 {
  margin: 0;
}

.hint {
  margin: 8px 0 18px;
  color: #4c5a67;
  font-size: 14px;
}

form {
  display: grid;
  gap: 12px;
}

label {
  display: grid;
  gap: 6px;
  font-size: 14px;
  color: #22313f;
}

input {
  border: 1px solid #c9d6e3;
  border-radius: 8px;
  padding: 10px 12px;
  font-size: 14px;
}

button {
  border: none;
  border-radius: 8px;
  background: #0d2b45;
  color: #fff;
  padding: 10px 12px;
  font-size: 14px;
  cursor: pointer;
}

button:disabled {
  opacity: 0.65;
  cursor: not-allowed;
}

.error {
  margin: 0;
  color: #b42318;
  font-size: 13px;
}
</style>
