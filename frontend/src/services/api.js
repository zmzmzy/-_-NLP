import axios from "axios";

const TOKEN_KEY = "gm_auth_token";

const api = axios.create({
  baseURL: "/api",
  timeout: 10000
});

api.interceptors.request.use((config) => {
  const token = localStorage.getItem(TOKEN_KEY);
  if (token) {
    config.headers = config.headers || {};
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

export default api;
