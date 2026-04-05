<template>
  <section class="auth-screen fade-up">
    <div class="auth-stack">
      <article class="auth-card auth-single">
        <header class="auth-head">
          <p class="small-text">Step 1</p>
          <h2>找回密码</h2>
        </header>
        <p class="hint">输入邮箱后，系统会通过邮件发送 6 位验证码。</p>
        <form class="auth-form" @submit.prevent="submitForgot">
          <label class="auth-field">
            <span>注册邮箱</span>
            <input v-model.trim="forgotForm.email" type="email" placeholder="name@example.com" />
          </label>
          <p v-if="forgotMsg" class="success">{{ forgotMsg }}</p>
          <button class="submit-btn" :disabled="forgotLoading" type="submit">
            {{ forgotLoading ? "提交中..." : "获取重置口令" }}
          </button>
        </form>
      </article>

      <article class="auth-card auth-single">
        <header class="auth-head">
          <p class="small-text">Step 2</p>
          <h2>重置密码</h2>
        </header>
        <p class="hint">填写邮箱、6 位验证码和新密码。</p>
        <form class="auth-form" @submit.prevent="submitReset">
          <label class="auth-field">
            <span>邮箱</span>
            <input v-model.trim="resetForm.email" type="email" placeholder="name@example.com" />
          </label>
          <label class="auth-field">
            <span>验证码</span>
            <input v-model.trim="resetForm.token" type="text" placeholder="输入邮件中的 6 位验证码" />
          </label>
          <label class="auth-field">
            <span>新密码</span>
            <input v-model="resetForm.newPassword" type="password" placeholder="至少 6 位" />
          </label>
          <label class="auth-field">
            <span>确认新密码</span>
            <input v-model="resetForm.confirmPassword" type="password" placeholder="再次输入" />
          </label>
          <p v-if="resetError" class="error">{{ resetError }}</p>
          <p v-if="resetMsg" class="success">{{ resetMsg }}</p>
          <button class="submit-btn" :disabled="resetLoading" type="submit">
            {{ resetLoading ? "提交中..." : "确认重置" }}
          </button>
        </form>
      </article>
    </div>

    <p class="auth-bottom">
      <router-link to="/login">返回登录</router-link>
      <span class="sep">·</span>
      <router-link to="/register">去注册</router-link>
    </p>
  </section>
</template>

<script setup>
import { reactive, ref } from "vue";
import { useRouter } from "vue-router";
import { useAuthStore } from "../stores/auth";

const router = useRouter();
const authStore = useAuthStore();

const forgotForm = reactive({
  email: ""
});
const resetForm = reactive({
  email: "",
  token: "",
  newPassword: "",
  confirmPassword: ""
});

const forgotLoading = ref(false);
const resetLoading = ref(false);
const forgotMsg = ref("");
const resetMsg = ref("");
const resetError = ref("");

async function submitForgot() {
  forgotMsg.value = "";
  if (!forgotForm.email) {
    forgotMsg.value = "请输入邮箱";
    return;
  }
  forgotLoading.value = true;
  try {
    const data = await authStore.forgotPassword(forgotForm.email);
    forgotMsg.value = data?.message || "请求已提交";
    if (!resetForm.email) {
      resetForm.email = forgotForm.email;
    }
  } catch (err) {
    forgotMsg.value = err?.response?.data?.error || "提交失败";
  } finally {
    forgotLoading.value = false;
  }
}

async function submitReset() {
  resetMsg.value = "";
  resetError.value = "";
  if (!resetForm.email || !resetForm.token || !resetForm.newPassword) {
    resetError.value = "邮箱、6 位验证码、新密码不能为空";
    return;
  }
  if (resetForm.newPassword.length < 6) {
    resetError.value = "新密码至少 6 位";
    return;
  }
  if (resetForm.newPassword !== resetForm.confirmPassword) {
    resetError.value = "两次密码输入不一致";
    return;
  }

  resetLoading.value = true;
  try {
    await authStore.resetPassword(resetForm.email, resetForm.token, resetForm.newPassword);
    resetMsg.value = "密码重置成功，正在跳转登录页...";
    setTimeout(() => {
      router.replace("/login");
    }, 500);
  } catch (err) {
    resetError.value = err?.response?.data?.error || "重置失败";
  } finally {
    resetLoading.value = false;
  }
}
</script>

<style scoped>
.auth-screen {
  min-height: min(860px, calc(100vh - 12px));
  display: grid;
  place-items: center;
  padding: 18px;
}

.auth-stack {
  width: min(560px, calc(100vw - 24px));
  display: grid;
  gap: 14px;
}

.auth-single {
  width: 100%;
}

.small-text {
  margin: 0;
  color: #7d8ca6;
  font-size: 13px;
}

h2 {
  margin: 6px 0 0;
  color: #f5f8ff;
  font-size: 30px;
}

.hint {
  margin: 0 0 14px;
  color: #9db0cb;
  font-size: 13px;
}

.auth-form {
  display: grid;
  gap: 12px;
}

.auth-field {
  display: grid;
  gap: 6px;
}

.auth-field span {
  color: #c9d6ec;
  font-size: 14px;
}

.error,
.success {
  margin: 0;
  font-size: 13px;
  word-break: break-word;
}

.error {
  color: #b42318;
}

.success {
  color: #027a48;
}

.auth-bottom {
  margin: 10px 0 0;
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
