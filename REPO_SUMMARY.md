# 仓库总览（topology-master-vc3）

下面是对仓库中各文件与目录的梳理说明（中文），便于快速理解项目结构与用途。

**概览**
- 本仓库是 `topology-master` 的一个工作副本，改动用于支持运行时可配置的虚拟通道（VC）数量。修改集中在 `topology-master-vc3` 分支/目录。

**构建与依赖**
- 使用 CMake 构建（`CMakeLists.txt`）。也包含旧式 `Makefile` 供直接编译使用。
- Python 依赖：`requirements.txt`（包含 `Flask>=2.0`，用于 `tools/web_gui.py`）。

**重要顶层文件**
- `README.md` / `README_DETAILED.md`：项目说明、设计目标、构建/运行示例与建议的后续改进（例如添加 `--vc` CLI 参数）。
- `CMakeLists.txt`：C 语言项目的 cmake 配置，检测 OpenMP 并设置编译选项。
- `Makefile`：一个简单的 make 脚本，使用 `gcc` 编译 `src/` 下的 `.c` 文件到 `bin/`。
- `requirements.txt`：Python 依赖（主要是 Flask）。
- `SUMMARY.md`：仓库内的另一个摘要文件（存在但内容未在此处列出）。

**目录说明（按重要性）**
- `src/`：核心 C 源码与若干 Python 工具脚本（主要开发和仿真逻辑）。主要文件：
  - `topology.c`：处理拓扑构造、路由结构等核心逻辑（与 `include/topology.h` 配合）。
  - `route_sim_anytopo.c`：任意拓扑路由模拟主逻辑，针对 VC 的运行时支持已被改造为动态分配。
  - `path_with_turn_forbidden.c`、`l_turn.c`、`octo_turn.c`、`tree_trun.c`：各种带有路径/转弯约束的路由算法实现。
  - `floyd_warshall.c`：用于最短路计算的实现（用于分析、路径生成等）。
  - `dfs.c`、`graph_analysis.c`：图分析和遍历工具。
  - `heap.c`：优先队列实现（用于 Dijkstra/路径计算）。
  - `utils.c`：工具函数集。
  - `globals.c`：本分支新增，定义运行时 VC 变量 `ROUTE_SIM_ANYTOPO_VC_RUNTIME` 等全局量。
  - `sa.c`、`sa_exchange.c`：模拟或启发式算法（可能是自适应路由或负载均衡相关）。
  - `traffic_gen.py`、`main.py`、`sa_gen_main.py`：Python 脚本，辅助生成流量或作为测试/运行入口（`main.py` 不是编译到 C 可执行的入口，注意区分）。

- `include/`：头文件集合，对应 `src/` 中的实现。重要头文件：
  - `route_sim_anytopo.h`：改动点之一，声明 `extern int ROUTE_SIM_ANYTOPO_VC_RUNTIME;`，用于在运行时确定 VC 数量。
  - `topology.h`、`topo_gen.h`：拓扑相关的结构与生成函数声明。
  - `graph_analysis.h`、`floyd_warshall.h`：图分析/路径算法声明。
  - 其它如 `l_turn.h`、`octo_turn.h`、`tree_turn.h` 等分别对应不同路由规则。

- `draw/`：绘图与日志解析脚本（Python），用于可视化结果或解析实验输出。例如：
  - `draw_sa.py`, `draw_bar.py`, `draw_deadlock.py`, `draw_adaptive_2vc3.py`：各类绘图脚本。
  - `parse_*_log.py`：解析对应实验日志并提取结果数据。
  - 子目录 `draw/path/`：路径相关绘图脚本，如 `draw_path_hist.py`、`draw_path_bar.py`。

- `scripts/`：实验与分析的辅助脚本（Python）：
  - `convert_to_topo.py`：把某些输入转换为本项目的拓扑格式。
  - `analyze_pathlog.py`、`analyze_deadlock_dump.py`：分析路径日志与死锁转储。
  - `validate_topo.py`：校验拓扑文件格式。
  - `organize_results.py`、`plot_path_log.py`：整理与画图辅助脚本。

- `tools/`：GUI/运行辅助工具：
  - `web_gui.py`：基于 Flask 的轻量 Web GUI（`requirements.txt` 列出了 Flask）。
  - `gui_run_topology.py`：可能封装了通过 GUI 启动实验的逻辑。

- `results/`：实验结果样本与生成的图/压缩包：
  - 包含 `slimfly_q17_radix36_1000_*` 系列文件、`ramanujan*`、以及 `extended_summary_from_vc3.txt` 等。可作为示例输入/输出查看。

- `patches/`：包含若干补丁与变更记录（`.patch`、`CHANGELOG.txt`、`COMMIT_MSG.txt`），方便复现或回滚修改。

- `temp/`：临时文件（示例、转换后的拓扑等），视情况可能为空或仅在本地开发时使用。

**如何快速构建与运行（建议）**
- 使用 CMake：

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target all
```

- 或使用仓库中的 `Makefile`（会把可执行放到 `bin/`）：

```bash
make
```

- 运行示例（参考 `README_DETAILED.md`）：

```bash
bin/main /tmp/small_topo3.txt 0 0 0 1000 1 0 0 0 /tmp/pathlog.txt /tmp/traffic.csv 8
```

（注意：仓库当前可能没有 `bin/main` 二进制——需先构建。）

**建议的后续小改动（摘自 README）**
- 为程序添加 CLI `--vc N` 或额外位置参数，以便直接通过命令行指定 `ROUTE_SIM_ANYTOPO_VC_RUNTIME`。
- 将临时批处理脚本从 `/tmp` 移入 `scripts/`，并把输出汇总写入 `results/`，便于版本控制与复现。

---

如果你需要，我可以：
- 把这份说明补充到更详细的 `REPO_SUMMARY.md`（已写入仓库根目录）。
- 实际构建一次并把 `bin/main` 产生（或修复构建问题）。
- 实现 `--vc` CLI 参数并做一次小规模测试。

请告诉我你接下来想让我做哪一步（例如：运行构建、实现 CLI、或把说明翻译成中文/英文的更正式文档）。