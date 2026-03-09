<template>
  <section class="login-page">
    <div class="login-card">
      <h2>系统登录</h2>
      <p class="hint">支持用户名或邮箱登录。</p>

      <form @submit.prevent="submitLogin">
        <label>
          账号（用户名 / 邮箱）
          <input v-model.trim="form.account" type="text" placeholder="请输入用户名或邮箱" />
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

      <p class="links">
        <router-link to="/register">注册账号</router-link>
        <span>|</span>
        <router-link to="/password">找回密码</router-link>
      </p>
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
  account: "",
  password: ""
});

const loading = ref(false);
const errorText = ref("");

async function submitLogin() {
  errorText.value = "";
  if (!form.account || !form.password) {
    errorText.value = "账号和密码不能为空";
    return;
  }

  loading.value = true;
  try {
    await authStore.login(form.account, form.password);
    const redirect = typeof route.query.redirect === "string" ? route.query.redirect : "/";
    router.replace(redirect);
  } catch (err) {
    errorText.value = err?.response?.data?.error || "登录失败，请检查账号";
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

.links {
  margin-top: 14px;
  font-size: 13px;
  color: #344054;
  display: flex;
  gap: 8px;
  align-items: center;
}

.links a {
  color: #0d2b45;
}
</style>
