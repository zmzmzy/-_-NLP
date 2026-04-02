# graduation_proj

毕业设计项目：`基于 NLP 的高校专业与就业岗位匹配分析系统`  
技术栈：`Vue 3 + Drogon(C++) + MySQL 8`

## 项目概览

当前主流程：

1. 学校/学院/专业/专业画像管理（含 CSV 导入）
2. 学生与就业信息管理（含学生自助提交 + 管理员审核）
3. 专业-岗位、学生-岗位匹配计算与结果解释
4. 决策看板（专业维度聚合评分与排名）
5. 注册/登录/找回密码（SMTP 发信）

说明：`tools/job_sync/` 为历史实验目录，不作为当前主流程依赖。

## 目录结构

- `frontend/`：前端（Vue + Vite）
- `backend/`：后端（Drogon + C++20）
- `db/`：数据库 Schema、Seed、迁移与验收 SQL
- `docs/`：需求、设计、测试、答辩与操作文档
- `tools/`：历史工具与实验文件

## 注意事项

### 前端

- 默认开发地址：`http://127.0.0.1:5173`
- `vite` 代理规则：`/api/* -> http://localhost:5555`
- 若需要同时登录“管理员+学生”进行联调，请使用不同浏览器或无痕窗口，避免同一会话 Cookie 覆盖
- 首次启动需执行 `npm install`；依赖异常可删除 `node_modules` 后重装

### 后端

- 使用 Drogon，要求 C++20 编译环境
- 默认监听端口：`5555`
- 数据库连接优先读 `backend/config/dev.json`，可通过 `GM_DB_*` 环境变量覆盖
- 启用 SMTP 时依赖系统 `curl`；`start_linux.sh`/`start_windows.bat` 会尝试加载 SMTP 环境文件

### 数据库

- 推荐 MySQL 8.x，字符集 `utf8mb4`
- 自增主键不保证连续（删除后不会回填），这是正常行为，不建议改
- `seed_full.sql` 和 `seed_minimal.sql` 均不再创建默认管理员账号
- 可按场景导入不同数据包：
  - 基础全量样本：`db/load_full_seed.sh`
  - 答辩展示样本：`db/defense_showcase_seed_v1.sql`

## 部署与运行须知

### 1) 环境准备

- MySQL 8.x
- CMake 3.16+
- Drogon（可被 CMake `find_package(Drogon)` 找到）
- Node.js + npm
- curl（仅 SMTP 场景必需）

### 2) 初始化数据库

方式 A（推荐，完整样本）：

```bash
cd /home/roamer/graduation_proj
./db/load_full_seed.sh
```

可用环境变量：`DB_HOST`、`DB_PORT`、`DB_USER`、`DB_PASS`

方式 B（仅建表）：

```bash
cd /home/roamer/graduation_proj
mysql -uroot -p123456 < db/schema.sql
```

方式 C（答辩演示数据包）：

```bash
cd /home/roamer/graduation_proj
mysql -uroot -p123456 < db/defense_showcase_seed_v1.sql
mysql -uroot -p123456 < db/defense_showcase_verify_v1.sql
```

### 3) 一键启动（推荐）

Linux:

```bash
cd /home/roamer/graduation_proj
chmod +x start_linux.sh
cp backend/.env.smtp.example backend/.env.smtp   # 可选
./start_linux.sh
```

Windows:

```bat
cd /d D:\path\to\graduation_proj
copy backend\.env.smtp.example.bat backend\.env.smtp.bat   REM 可选
start_windows.bat
```

可选环境变量：

- `FRONTEND_PORT`：前端端口（默认 `5173`）
- `FORCE_NPM_INSTALL=1`：强制重新安装前端依赖
- `SMTP_ENV_FILE`：SMTP 环境文件路径（默认 backend 下示例文件）

### 4) 手动启动

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

### 5) 启动后检查

- 后端健康检查：`GET http://127.0.0.1:5555/api/health`
- 前端入口：`http://127.0.0.1:5173`
- 首次使用请先注册账号，默认角色为 `viewer`
- 提升管理员权限示例：
  - `UPDATE users SET role='admin' WHERE email='你的邮箱';`

## 文档导航

- 主索引：`docs/doc_index.md`
- 测试手册：`docs/testing_guide.md`
- 后端接口：`backend/docs/api_contract.md`
- 后端启动细节：`backend/docs/setup.md`
- 前端启动细节：`frontend/docs/setup.md`
- 答辩数据包手册：`docs/defense_showcase_manual_v1.md`

## 自动化测试（新增）

可直接执行：

```bash
cd /home/roamer/graduation_proj
chmod +x tools/testing/run_local_tests.sh
./tools/testing/run_local_tests.sh
```

该脚本会依次执行：

- 后端重新编译
- 数据库重建与种子导入
- 后端服务拉起与健康检查
- API 集成冒烟测试
- API 异常路径冒烟测试
- 数据库验收报告

详细说明见：`tools/testing/README.md`
