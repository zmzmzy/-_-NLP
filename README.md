# graduation_proj

毕业设计项目（v0.3 精简版）  
主题：`基于 NLP 的高校专业与社会岗位匹配分析系统`

## 项目范围（当前冻结）

1. 结构化管理专业数据（多学校/学院/专业）
2. 结构化管理企业与岗位数据
3. 支持专业与岗位的 CSV 导入（预检 + 应用）
4. 计算并解释“专业-岗位”匹配度（主流程）
5. 学生与就业记录模块保留为扩展能力

> 已下线：岗位爬取/岗位同步脚本与 `/api/job-sync/*` 接口，不再作为当前主流程。

## 目录结构

- `docs/`：需求、设计、测试与执行 TODO
- `db/`：MySQL 建表、种子数据、迁移脚本
- `backend/`：C++ Drogon 后端
- `frontend/`：Vue 前端
- `tools/`：历史工具与实验文件（不属于当前主流程）

## 快速启动

### 1. 初始化数据库

```bash
cd /home/roamer/graduation_proj
./db/load_full_seed.sh
```

可用环境变量：`DB_HOST`、`DB_PORT`、`DB_USER`、`DB_PASS`

### 2. 一键启动（推荐）

Linux:

```bash
cd /home/roamer/graduation_proj
chmod +x start_linux.sh
# 可选：先准备 SMTP 配置
cp backend/.env.smtp.example backend/.env.smtp
# 然后编辑 backend/.env.smtp 填入授权码
./start_linux.sh
```

Windows:

```bat
cd /d D:\path\to\graduation_proj
REM 可选：先准备 SMTP 配置
copy backend\.env.smtp.example.bat backend\.env.smtp.bat
REM 然后编辑 backend\.env.smtp.bat 填入授权码
start_windows.bat
```

可选环境变量：

- `FRONTEND_PORT`：前端端口（默认 `5173`）
- `FORCE_NPM_INSTALL=1`：强制重新安装前端依赖
- `SMTP_ENV_FILE`：自定义 SMTP 环境文件路径

### 3. 手动启动

后端：

```bash
cd /home/roamer/graduation_proj/backend
cmake -S . -B build
cmake --build build -j
./build/graduate_match_backend
```

前端：

```bash
cd /home/roamer/graduation_proj/frontend
npm install
npm run dev
```

## 账号初始化说明

- 项目已取消默认账号写入（`db/seed_full.sql` 与 `db/seed_minimal.sql` 不再创建 `admin`）。
- 请先在登录页点击“注册账号”，默认角色为 `viewer`。
- 如需管理权限，请手动执行 SQL 提升角色：`UPDATE users SET role='admin' WHERE email='你的邮箱';`

## 当前版本状态

- 主链路：专业管理 + 岗位管理 + 匹配分析 + 看板，已可运行
- 专业导入：`/api/majors/import/preview|apply|batches`
- 岗位导入：`/api/jobs/import/preview|apply|batches`
- 后端健康检查：`GET /api/health`（版本 `0.3.0`）

## 文档入口

- `docs/doc_index.md`
- `docs/testing_guide.md`
- `backend/docs/api_contract.md`
- `backend/docs/setup.md`
