const { defineConfig } = require("vite");
const vue = require("@vitejs/plugin-vue");
const fs = require("fs");
const path = require("path");

function resolveHttpsOption() {
  const disabled = String(process.env.VITE_DISABLE_HTTPS || "").toLowerCase();
  if (disabled === "1" || disabled === "true") {
    return false;
  }

  const customKey = process.env.VITE_HTTPS_KEY;
  const customCert = process.env.VITE_HTTPS_CERT;
  if (customKey && customCert) {
    const keyPath = path.resolve(__dirname, customKey);
    const certPath = path.resolve(__dirname, customCert);
    if (fs.existsSync(keyPath) && fs.existsSync(certPath)) {
      return {
        key: fs.readFileSync(keyPath),
        cert: fs.readFileSync(certPath)
      };
    }
    console.warn("[vite] custom HTTPS certificate not found, fallback to auto-generated certificate.");
  }

  const defaultKey = path.resolve(__dirname, "certs/localhost-key.pem");
  const defaultCert = path.resolve(__dirname, "certs/localhost.pem");
  if (fs.existsSync(defaultKey) && fs.existsSync(defaultCert)) {
    return {
      key: fs.readFileSync(defaultKey),
      cert: fs.readFileSync(defaultCert)
    };
  }

  return true;
}

const httpsOption = resolveHttpsOption();

module.exports = defineConfig({
  plugins: [vue()],
  server: {
    host: "127.0.0.1",
    port: 5173,
    https: httpsOption,
    proxy: {
      "/api": {
        target: "http://127.0.0.1:5555",
        changeOrigin: true,
        timeout: 120000,
        proxyTimeout: 120000
      }
    }
  },
  preview: {
    host: "127.0.0.1",
    port: 4173,
    https: httpsOption
  }
});
