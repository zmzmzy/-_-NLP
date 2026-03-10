# 答辩展示数据包操作手册（v1）

## 1. 数据包内容

- 种子导入：`db/defense_showcase_seed_v1.sql`
- 一键回滚：`db/defense_showcase_cleanup_v1.sql`
- 验收检查：`db/defense_showcase_verify_v1.sql`

该数据包用于毕业设计答辩演示，导入后可得到：

- 10 个学院（`DC01`~`DC10`）
- 20 个专业（`DM001`~`DM020`）
- 100 个学生（`D26S0001`~`D26S0100`，每专业 5 人）
- 10 个公司 + 20 个岗位
- 100 条就业记录（含“较对口样本”与“跨域样本”）

## 2. 文本质量约束（已内置）

- 专业描述与专业画像文本：每条不少于 150 字
- 公司文本、岗位文本、就业说明文本：每条不少于 100 字
- 学生简历/项目/能力文本：每条不少于 100 字

## 3. 导入步骤

在项目根目录执行：

```bash
cd /home/roamer/graduation_proj
mysql -uroot -p123456 < db/defense_showcase_seed_v1.sql
```

如果你的 MySQL 账号密码不是 `root/123456`，请替换成你本机实际账号。

## 4. 验收步骤（强烈建议）

```bash
cd /home/roamer/graduation_proj
mysql -uroot -p123456 < db/defense_showcase_verify_v1.sql
```

看到各检查项 `status=PASS` 即说明数据规模和文本长度达标。

## 5. 演示账号

- 管理员：
  - 用户名：`def_admin`
  - 密码：`Demo@123`
- 学生演示账号（10个）：
  - 用户名：`def_stu001` ~ `def_stu010`
  - 密码统一：`Demo@123`
  - 已绑定学生：`D26S0001` ~ `D26S0010`

## 6. 推荐答辩演示流程

1. 管理员登录，展示学院/专业/学生总量达标（10/20/100）。
2. 打开任一专业详情，展示“培养方案文本 + 学生就业文本”均为长文本。
3. 进入匹配分析页面，运行专业维度/学院维度匹配计算。
4. 打开决策看板，展示不同专业“优秀毕业生分数”与排名差异。
5. 选取一个“较对口样本”和一个“跨域样本”，解释分数差异的可解释性。

## 7. 回滚（清理答辩数据）

```bash
cd /home/roamer/graduation_proj
mysql -uroot -p123456 < db/defense_showcase_cleanup_v1.sql
```

该脚本只删除带答辩标记的数据（`DC*`、`D26S*`、`def_*`、`（答辩）`），不影响其他业务数据。
