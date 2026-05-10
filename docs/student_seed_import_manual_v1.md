# 学生数据自动化导入手册（v1）

适用目录：`/home/roamer/graduation_proj_0.1`

## 1. 能力说明

工具路径：

- `tools/student_seed/import_students_seed.py`
- `tools/student_seed/run_student_seed.sh`

功能：

1. 自动读取数据库已有 `colleges + majors`
2. 自动生成每专业 `6-12` 人学生数据
3. 学号按 `222022 + 学院3位 + 专业3位 + 班级学号3位` 生成（共15位）
4. 输出导入计划 CSV、入库 SQL、回滚 SQL、验收 CSV
5. 支持幂等导入（重复执行不会因学号冲突报错）

## 2. 快速使用

### 2.1 干跑（只生成文件，不写库）

```bash
cd /home/roamer/graduation_proj_0.1
./tools/student_seed/run_student_seed.sh dry-run
```

输出文件位于 `import_samples/`：

- `students_import_plan_*.csv`
- `students_import_apply_*.sql`
- `students_import_rollback_*.sql`

### 2.2 正式导入（写库）

```bash
cd /home/roamer/graduation_proj_0.1
./tools/student_seed/run_student_seed.sh apply
```

导入成功后会新增：

- `students_import_verify_*.csv`（每专业人数校验）

## 3. 参数约束

默认参数：

- 每专业人数：`6-12`（循环分配）
- 学号前缀：`222022`
- 毕业年份：`2026`
- 专业3位段：默认取 `major_code` 后3位（`tail3`）

可通过环境变量覆盖：

```bash
export MIN_PER_MAJOR=6
export MAX_PER_MAJOR=12
export YEAR_PREFIX=222022
export GRAD_YEAR=2026
export DIST=cycle            # cycle 或 fixed
export FIXED_PER_MAJOR=8     # DIST=fixed 时生效
export MAJOR_SEGMENT_MODE=tail3  # tail3 或 seq3
```

## 4. 学号段自定义（可选）

如果你不想用 `major_code` 后3位，可以提供映射 CSV：

- 文件格式：`major_code,major_segment`
- 例：`085401,072`

执行示例：

```bash
cd /home/roamer/graduation_proj_0.1
export MAJOR_SEGMENT_MAP_CSV=/abs/path/major_segment_map.csv
./tools/student_seed/run_student_seed.sh apply
```

## 5. 回滚

导入后如果需要撤销，执行同批次生成的回滚文件：

```bash
mysql -uroot -p123456 < import_samples/students_import_rollback_<tag>_<timestamp>.sql
```

## 6. 验收建议

### 6.1 每专业人数检查（6-12）

```sql
USE graduate_match;
SELECT m.code, m.name, COUNT(s.id) AS student_cnt
FROM majors m
LEFT JOIN students s ON s.major_id = m.id
GROUP BY m.id, m.code, m.name
HAVING student_cnt < 6 OR student_cnt > 12;
```

### 6.2 学号重复检查

```sql
USE graduate_match;
SELECT student_no, COUNT(*) AS c
FROM students
GROUP BY student_no
HAVING c > 1;
```
