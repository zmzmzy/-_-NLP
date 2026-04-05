# HTTPS Dev Setup

The frontend dev server now starts with HTTPS by default.

## Run

```bash
npm --prefix frontend run dev
```

Open: `https://127.0.0.1:5173`

## Optional: run HTTP temporarily

```bash
npm --prefix frontend run dev:http
```

## Use your own certificate (recommended for trusted local HTTPS)

Set environment variables before starting Vite:

```bash
export VITE_HTTPS_KEY=certs/localhost-key.pem
export VITE_HTTPS_CERT=certs/localhost.pem
npm --prefix frontend run dev
```

If the files are not provided, Vite falls back to an auto-generated certificate.

## Disable HTTPS via env

```bash
VITE_DISABLE_HTTPS=1 npm --prefix frontend run dev
```

## One-Click Script (`start_linux.sh`)

`start_linux.sh` now starts frontend with HTTPS by default.

```bash
./start_linux.sh
```

Switch to HTTP:

```bash
FRONTEND_USE_HTTPS=0 ./start_linux.sh
```
