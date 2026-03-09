<template>
  <section class="auth-page">
    <div class="auth-wrap">
      <article class="auth-card">
        <h2>找回密码</h2>
        <p class="hint">输入邮箱后，系统会通过邮件发送 6 位验证码。</p>
        <form @submit.prevent="submitForgot">
          <label>
            注册邮箱
            <input v-model.trim="forgotForm.email" type="email" placeholder="name@example.com" />
          </label>
          <p v-if="forgotMsg" class="success">{{ forgotMsg }}</p>
          <button :disabled="forgotLoading" type="submit">
            {{ forgotLoading ? "提交中..." : "获取重置口令" }}
          </button>
        </form>
      </article>

      <article class="auth-card">
        <h2>重置密码</h2>
        <p class="hint">填写邮箱、6 位验证码和新密码。</p>
        <form @submit.prevent="submitReset">
          <label>
            邮箱
            <input v-model.trim="resetForm.email" type="email" placeholder="name@example.com" />
          </label>
          <label>
            验证码
            <input v-model.trim="resetForm.token" type="text" placeholder="输入邮件中的 6 位验证码" />
          </label>
          <label>
            新密码
            <input v-model="resetForm.newPassword" type="password" placeholder="至少 6 位" />
          </label>
          <label>
            确认新密码
            <input v-model="resetForm.confirmPassword" type="password" placeholder="再次输入" />
          </label>
          <p v-if="resetError" class="error">{{ resetError }}</p>
          <p v-if="resetMsg" class="success">{{ resetMsg }}</p>
          <button :disabled="resetLoading" type="submit">
            {{ resetLoading ? "提交中..." : "确认重置" }}
          </button>
        </form>
      </article>
    </div>

    <p class="links">
      <router-link to="/login">返回登录</router-link>
      <span>|</span>
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
.auth-page {
  min-height: calc(100vh - 40px);
  display: grid;
  place-items: center;
  padding: 24px 0;
}

.auth-wrap {
  width: min(980px, 94vw);
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.auth-card {
  background: #fff;
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 10px 28px rgba(13, 43, 69, 0.12);
}

h2 {
  margin: 0;
}

.hint {
  margin: 8px 0 14px;
  color: #4c5a67;
  font-size: 13px;
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

.success {
  margin: 0;
  color: #027a48;
  font-size: 13px;
  word-break: break-all;
}

.links {
  margin-top: 12px;
  font-size: 13px;
  color: #344054;
  display: flex;
  gap: 8px;
  align-items: center;
}

.links a {
  color: #0d2b45;
}

@media (max-width: 900px) {
  .auth-wrap {
    grid-template-columns: 1fr;
  }
}
</style>
