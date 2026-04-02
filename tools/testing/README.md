# 本地自动化测试说明

目录包含三类脚本：

- `run_local_tests.sh`：一键执行本地回归（重建数据、启动后端、跑集成和异常测试）
- `api_integration_smoke.py`：主业务链路集成冒烟测试
- `api_negative_smoke.py`：异常路径冒烟测试（权限、参数、外键约束等）

## 快速使用

```bash
cd /home/roamer/graduation_proj
chmod +x tools/testing/run_local_tests.sh
./tools/testing/run_local_tests.sh
```

## 前置条件

- MySQL 可用，账号 `root`，密码 `123456`
- 可执行命令：`cmake`、`mysql`、`python3`
- 后端默认监听 `127.0.0.1:5555`

## 注意事项

- 脚本会重置并覆盖 `graduate_match` 数据库中的业务数据，请勿在生产库执行。
- 若本机开启代理，请确保 `127.0.0.1` 走直连（脚本内部已强制禁用代理访问本地地址）。
