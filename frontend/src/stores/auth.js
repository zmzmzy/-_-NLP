import { defineStore } from "pinia";
import api from "../services/api";

const TOKEN_KEY = "gm_auth_token";
const USER_KEY = "gm_auth_user";
const EXPIRES_KEY = "gm_auth_expires_at";

function readUserFromStorage() {
  const raw = localStorage.getItem(USER_KEY);
  if (!raw) {
    return null;
  }
  try {
    return normalizeUserPayload(JSON.parse(raw));
  } catch (_) {
    return null;
  }
}

function normalizeUserPayload(user) {
  if (!user || typeof user !== "object") {
    return null;
  }
  return {
    id: Number(user.id || 0),
    username: user.username || "",
    email: user.email || "",
    role: user.role || "viewer",
    is_active: Boolean(user.is_active ?? true),
    linked_student_id: user.linked_student_id ?? null
  };
}

export const useAuthStore = defineStore("auth", {
  state: () => ({
    token: localStorage.getItem(TOKEN_KEY) || "",
    user: readUserFromStorage(),
    expiresAtEpoch: Number(localStorage.getItem(EXPIRES_KEY) || 0),
    bootstrapped: false
  }),
  getters: {
    isAuthenticated(state) {
      return !!state.token;
    }
  },
  actions: {
    setSession(token, user, expiresAtEpoch) {
      this.token = token;
      this.user = normalizeUserPayload(user);
      this.expiresAtEpoch = Number(expiresAtEpoch || 0);
      localStorage.setItem(TOKEN_KEY, this.token);
      localStorage.setItem(USER_KEY, JSON.stringify(this.user || {}));
      localStorage.setItem(EXPIRES_KEY, String(this.expiresAtEpoch || 0));
    },
    clearSession() {
      this.token = "";
      this.user = null;
      this.expiresAtEpoch = 0;
      localStorage.removeItem(TOKEN_KEY);
      localStorage.removeItem(USER_KEY);
      localStorage.removeItem(EXPIRES_KEY);
    },
    async login(account, password) {
      const resp = await api.post("/auth/login", { account, password });
      const data = resp?.data?.data || {};
      this.setSession(data.token || "", data.user || null, data.expires_at_epoch || 0);
      return data;
    },
    async register(payload) {
      const resp = await api.post("/auth/register", payload || {});
      const data = resp?.data?.data || {};
      this.setSession(data.token || "", data.user || null, data.expires_at_epoch || 0);
      return data;
    },
    async forgotPassword(email) {
      const resp = await api.post("/auth/password/forgot", { email });
      return resp?.data?.data || {};
    },
    async resetPassword(email, token, newPassword) {
      const resp = await api.post("/auth/password/reset", {
        email,
        token,
        new_password: newPassword
      });
      return resp?.data || {};
    },
    async fetchMe() {
      const resp = await api.get("/auth/me");
      const data = resp?.data?.data || null;
      if (data) {
        this.user = normalizeUserPayload({
          id: data.id,
          username: data.username,
          email: data.email,
          role: data.role,
          is_active: data.is_active,
          linked_student_id: data.linked_student_id
        });
        this.expiresAtEpoch = Number(data.expires_at_epoch || this.expiresAtEpoch || 0);
        localStorage.setItem(USER_KEY, JSON.stringify(this.user));
        localStorage.setItem(EXPIRES_KEY, String(this.expiresAtEpoch || 0));
      }
      return data;
    },
    async logout() {
      try {
        if (this.token) {
          await api.post("/auth/logout");
        }
      } finally {
        this.clearSession();
      }
    },
    async bootstrap() {
      if (this.bootstrapped) {
        return;
      }
      this.bootstrapped = true;
      if (!this.token) {
        return;
      }
      try {
        await this.fetchMe();
      } catch (_) {
        this.clearSession();
      }
    }
  }
});
