# 后端启动说明

## 编译

```bash
cd backend
cmake -S . -B build
cmake --build build -j
```

## 启动

```bash
cd backend/build
./graduate_match_backend
```

## 验证

```bash
curl http://127.0.0.1:5555/api/health
```

期望返回：

```json
{"status":"ok","service":"graduate_match_backend","version":"0.1.0"}
```

## 认证快速验证

默认测试账号（见 `db/seed_full.sql`）：

- 用户名：`admin`
- 密码：`admin123`

如需一键准备完整样本数据：

```bash
cd /home/roamer/graduation_proj_0.1
./db/load_full_seed.sh
```

```bash
curl -s -X POST http://127.0.0.1:5555/api/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"username":"admin","password":"admin123"}'
```
