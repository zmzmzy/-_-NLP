<template>
  <section class="auth-screen fade-up">
    <div class="auth-card auth-single">
      <header class="auth-head">
        <p class="small-text">Welcome back</p>
        <h2>账号登录</h2>
      </header>

      <form class="auth-form" @submit.prevent="submitLogin">
        <label class="auth-field">
          <span>账号（用户名 / 邮箱）</span>
          <input v-model.trim="form.account" type="text" placeholder="请输入用户名 / 学号 / 邮箱" />
        </label>

        <label class="auth-field">
          <span>密码</span>
          <input v-model="form.password" type="password" placeholder="请输入密码" />
        </label>

        <div class="auth-row">
          <label class="auth-check">
            <input v-model="remember" type="checkbox" />
            <span>记住我</span>
          </label>
          <router-link class="auth-link" to="/password">忘记密码？</router-link>
        </div>

        <p v-if="errorText" class="error">{{ errorText }}</p>

        <button class="submit-btn" :disabled="loading" type="submit">
          {{ loading ? "登录中..." : "立即登录" }}
        </button>
      </form>

      <p class="auth-bottom">
        还没有账号？
        <router-link to="/register">立即注册</router-link>
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
const remember = ref(true);

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
.auth-screen {
  min-height: min(760px, calc(100vh - 12px));
  display: grid;
  place-items: center;
  padding: 18px;
}

.auth-single {
  width: min(520px, calc(100vw - 24px));
}

.auth-head {
  margin-bottom: 16px;
}

.small-text {
  margin: 0;
  color: #7d8ca6;
  font-size: 13px;
}

h2 {
  margin: 6px 0 0;
  font-size: 34px;
  color: #f5f8ff;
}

.auth-form {
  display: grid;
  gap: 14px;
}

.auth-field {
  display: grid;
  gap: 6px;
}

.auth-field span {
  font-size: 14px;
  color: #c9d6ec;
}

.auth-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  font-size: 13px;
}

.auth-check {
  display: flex;
  align-items: center;
  gap: 8px;
  color: #9db0cb;
}

.auth-link {
  text-decoration: none;
  color: #8db4ff;
}

.auth-link:hover {
  color: #b3cfff;
}

.auth-bottom {
  margin: 16px 0 0;
  text-align: center;
  color: #9db0cb;
  font-size: 13px;
}

.auth-bottom a {
  color: #8db4ff;
  text-decoration: none;
  font-weight: 600;
}

.auth-bottom a:hover {
  color: #b3cfff;
}
</style>
