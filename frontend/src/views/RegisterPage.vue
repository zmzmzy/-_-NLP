<template>
  <section class="auth-page">
    <div class="auth-card">
      <h2>邮箱注册</h2>
      <p class="hint">注册后自动登录，默认角色为普通用户（viewer）。</p>

      <form @submit.prevent="submitRegister">
        <label>
          邮箱
          <input v-model.trim="form.email" type="email" placeholder="name@example.com" />
        </label>

        <label>
          用户名（可选）
          <input v-model.trim="form.username" type="text" placeholder="留空自动生成" />
        </label>

        <label>
          密码
          <input v-model="form.password" type="password" placeholder="至少 6 位" />
        </label>

        <label>
          确认密码
          <input v-model="form.confirmPassword" type="password" placeholder="再次输入密码" />
        </label>

        <p v-if="errorText" class="error">{{ errorText }}</p>
        <p v-if="successText" class="success">{{ successText }}</p>

        <button :disabled="loading" type="submit">
          {{ loading ? "注册中..." : "注册并登录" }}
        </button>
      </form>

      <p class="links">
        <router-link to="/login">返回登录</router-link>
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
.auth-page {
  min-height: calc(100vh - 40px);
  display: grid;
  place-items: center;
}

.auth-card {
  width: min(460px, 92vw);
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

.success {
  margin: 0;
  color: #027a48;
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
