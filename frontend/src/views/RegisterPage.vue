<template>
  <section class="auth-screen fade-up">
    <div class="auth-card auth-single">
      <header class="auth-head">
        <p class="small-text">Get started</p>
        <h2>邮箱注册</h2>
      </header>

      <p class="hint">注册后自动登录，默认角色为普通用户（viewer）。</p>

      <form class="auth-form" @submit.prevent="submitRegister">
        <label class="auth-field">
          <span>邮箱</span>
          <input v-model.trim="form.email" type="email" placeholder="name@example.com" />
        </label>

        <label class="auth-field">
          <span>用户名（可选）</span>
          <input v-model.trim="form.username" type="text" placeholder="留空自动生成" />
        </label>

        <label class="auth-field">
          <span>密码</span>
          <input v-model="form.password" type="password" placeholder="至少 6 位" />
        </label>

        <label class="auth-field">
          <span>确认密码</span>
          <input v-model="form.confirmPassword" type="password" placeholder="再次输入密码" />
        </label>

        <p v-if="errorText" class="error">{{ errorText }}</p>
        <p v-if="successText" class="success">{{ successText }}</p>

        <button class="submit-btn" :disabled="loading" type="submit">
          {{ loading ? "注册中..." : "注册并登录" }}
        </button>
      </form>

      <p class="auth-bottom">
        已有账号？
        <router-link to="/login">返回登录</router-link>
        <span class="sep">·</span>
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
  email: "",
  username: "",
  password: "",
  confirmPassword: ""
});

const loading = ref(false);
const errorText = ref("");
const successText = ref("");

async function submitRegister() {
  errorText.value = "";
  successText.value = "";
  if (!form.email || !form.password) {
    errorText.value = "邮箱和密码不能为空";
    return;
  }
  if (form.password.length < 6) {
    errorText.value = "密码至少 6 位";
    return;
  }
  if (form.password !== form.confirmPassword) {
    errorText.value = "两次输入密码不一致";
    return;
  }

  loading.value = true;
  try {
    await authStore.register({
      email: form.email,
      username: form.username || undefined,
      password: form.password
    });
    successText.value = "注册成功，正在跳转...";
    const redirect = typeof route.query.redirect === "string" ? route.query.redirect : "/profile";
    setTimeout(() => {
      router.replace(redirect);
    }, 300);
  } catch (err) {
    errorText.value = err?.response?.data?.error || "注册失败";
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
  width: min(540px, calc(100vw - 24px));
}

.auth-head {
  margin-bottom: 12px;
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

.hint {
  margin: 0 0 16px;
  color: #9db0cb;
  font-size: 13px;
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

.sep {
  margin: 0 8px;
  color: #6f84a5;
}
</style>
