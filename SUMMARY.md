## 会话与实验总结（中文）

以下为对本次交互与在仓库 `topology-master` 上完成工作的完整总结，包含构建与运行步骤、代码改动、已执行命令与关键输出、当前状态以及后续建议。

---

### 1) 会话概览
- 主要目标：
  - 指导如何构建与运行 `topology-master` 项目（给出一步步顺序）。
  - 验证并运行用户提供的 Ramanujan 图（文件位于 `temp/ramanujan.txt`），在模拟器上进行仿真实验（用户指定 VC=2、LUT 模式=2；ppp 与 seed 由代理选择）。
  - 在仿真结束后分析并可视化 path-log（每节点路径计数 / 负载分布 / 热点检测等）。
- 会话流程简述：
  - 检查并构建项目（使用原始 `Makefile`，后改用并修正后的 `CMakeLists.txt`）。
  - 运行并调试二进制（修复了运行时断言问题：固定队列溢出 -> 动态扩容队列）。
  - 添加并使用 Python 工具链（在 conda 环境 `TOPO` 中安装 `numpy`、`matplotlib`）完成绘图与分析。
  - 将用户提供的邻接矩阵 `temp/ramanujan.txt` 转换为仓库期望的 topo 格式、验证连通性、做谱半径（lambda2）检查，结果通过 Ramanujan 上界测试。
  - 运行一次仿真（seed0）的 path-log 分析并生成 CSV、摘要文本和热图。

### 2) 技术栈、关键文件与环境
- 语言与工具：C（C11）、OpenMP、CMake、Make、Python3、NumPy、Matplotlib、conda 环境。
- 关键源码：
  - `src/topology.c`：程序入口与参数解析，调用 `route_sim_anytopo`。
  - `src/route_sim_anytopo.c` 与 `include/route_sim_anytopo.h`：路由仿真核心，宏定义了 VC 数（`ROUTE_SIM_ANYTOPO_VC=2`）。
  - `src/path_with_turn_forbidden.c`：路径枚举（已修改队列实现以避免固定大小断言）。
- 新增/修改脚本：
  - `scripts/convert_to_topo.py`：把邻接矩阵/列表转换为 repo topo 格式（`N,deg` + 每行邻居列表）。
  - `scripts/validate_topo.py`：检查 topo 文件格式与连通性（BFS）。
  - `scripts/check_ramanujan.py`：构造邻接矩阵，计算特征值并与 Ramanujan 上界比较。
  - `scripts/plot_path_log.py`：绘制 path-log 的 2D/1D 热图（依赖 numpy/matplotlib）。
  - `scripts/analyze_pathlog.py`：计算每节点总计、摘要统计（max/min/mean/median/std/top10/Gini），写 CSV/summary，并调用绘图。
- 运行环境：
  - 可执行文件：`bin/main`（从 `build/topology` 复制而来）。
  - Python 分析环境：conda env `TOPO`（python=3.10，numpy，matplotlib）。

### 3) 代码/构建变更摘要
- `CMakeLists.txt`：重写以使用 C11、正确查找与链接 OpenMP、链接 math 库（-lm）。
- `src/path_with_turn_forbidden.c`：在 `trun_model_dijkstra` 中将固定大小队列与断言替换为动态可增长队列（realloc 双倍扩容），修复了仿真中断言崩溃问题。
- 新增 Python 脚本（见上文列表）。

### 4) 已执行的关键命令与结果（精简）
- 构建与复制可执行：
  - `cp build/topology bin/main && chmod +x bin/main`
- 运行 torus 示例：
  - `bin/main temp/torus_10_10.txt 0 1 0 10 1 2 0 0 temp/torus_path_log.txt temp/dummy.csv 100`
  - 输出（示意）：Graph name..., Traffic Mode : 0, Route Lut Mode : 1, Num of VC : 2, Total cycle : 12, Avg Packet Delivery Latency : 8.08
- 绘图脚本初次失败（缺 numpy） -> 创建 conda env 并安装：
  - `conda create -y -n TOPO python=3.10 numpy matplotlib`
- 在 `TOPO` 下成功绘图：
  - `conda run -n TOPO python3 scripts/plot_path_log.py temp/torus_path_log.txt temp/torus_path_heatmap.png 10`
  - 输出：Saved 2D heatmap temp/torus_path_heatmap.png (10x10)
- 转换并验证 Ramanujan 图：
  - `conda run -n TOPO python3 scripts/convert_to_topo.py temp/ramanujan.txt temp/ramanujan_converted.txt`
    - 输出：Converted adjacency-matrix -> topo. N=1092, regular=True, min_deg=24, max_deg=24
  - `conda run -n TOPO python3 scripts/validate_topo.py temp/ramanujan_converted.txt`
    - 输出：Parsed N=1092, deg=24; Graph is connected
- 谱检验（Ramanujan bound）：
  - `conda run -n TOPO python3 scripts/check_ramanujan.py temp/ramanujan_converted.txt`
    - 输出节选：lambda1 ≈ 24.0, lambda2 ≈ 8.6846584, bound = 2 * sqrt(24-1) ≈ 9.5916630, lambda2 <= bound ? True
- path-log 分析（seed0）：
  - `mkdir -p results/ramanujan && conda run -n TOPO python3 scripts/analyze_pathlog.py temp/ramanujan_pathlog_seed0.txt results/ramanujan/seed0 33`
    - 输出：Saved 1D bar plot results/ramanujan/seed0_heatmap.png (N=1092)
    - 统计：max=217, mean≈11.8104, median=5, std≈21.6411, Gini≈0.644979，Top10 节点列出
    - 生成文件：`results/ramanujan/seed0_pernode.csv`, `results/ramanujan/seed0_summary.txt`, `results/ramanujan/seed0_heatmap.png`

### 5) 当前状态（已完成 / 未完成）
- 已完成：
  - 项目可编译并生成 `bin/main`（CMake 修正）。
  - 修复运行时队列溢出 bug。
  - 将 `temp/ramanujan.txt` 转换为仓库 topo 格式并验证为 1092 节点 24-正则图且连通。
  - 谱检验通过（lambda2 ≤ Ramanujan 上界）。
  - 运行并分析了 seed0 的 path-log，输出 CSV/summary/图像。
- 未完成 / 建议：
  - 按用户要求完成仿真扫参（VC=2、LUT=2）在 seeds [0,1,2] 与 ppp（建议 ppp=10）上运行，并对多次 seed 的输出进行汇总比较（均值/方差/热力图平均或 top-k 一致性）。
  - 可选：在仓库根目录增加 `SUMMARY.md` 或更新 `README.md` 记录运行步骤与常用脚本。

### 6) 下一步计划（短期可执行项）
- 立刻执行：按参数（VC=2、LUT=2、ppp=10）对 seeds 1 和 2 运行仿真，保留生成的 path-log（例如 `temp/ramanujan_pathlog_seed1.txt`、`temp/ramanujan_pathlog_seed2.txt`）。
- 对所有 seeds 运行 `scripts/analyze_pathlog.py` 并把结果汇总为 `results/ramanujan/summary_across_seeds.txt`（包含每个 seed 的 max/mean/Gini 及跨 seed 的均值/标准差）。
- 将这份完整会话总结写入仓库（此文件即为写入版本）。

### 7) 可选任务（我可以代劳）
- 现在运行剩余的 seeds 并输出汇总报告（我可以现在就运行）。
- 把这份完整稿保存为其他文件名（例如 `README.update.md`）或创建 Git 提交（需要你授权）。
- 生成对比测试（例如 torus 与随机 24-正则图）并输出图/表对比。

---

此 `SUMMARY.md` 文件由交互代理生成并写入仓库根目录，用于快速回顾本次工作与便于后续实验复现。

### 附：结果文件组织约定（已更新）
- 为减少结果目录的冗余与便于归档，分析脚本 `scripts/analyze_pathlog.py` 现在支持把输出写入以 `results/<case>/` 为前缀的目录。使用示例：
  - `conda run -n TOPO python3 scripts/analyze_pathlog.py temp/ramanujan_pathlog_seed0.txt results/ramanujan/seed0 33`
  - 上述命令将在 `results/ramanujan/seed0/` 下生成：
    - `pernode.csv`（每节点每方向计数 + total）
    - `summary.txt`（摘要统计与 top-k）
    - `heatmap.png`（2D/1D 热图）
- 旧文件（如 `results/ramanujan/seed0_pernode.csv`）已移入对应子目录以保持根目录整洁；如果需要恢复到旧命名风格，请把 `out_prefix` 设为带文件前缀（例如 `results/ramanujan/seed0` 将写入目录，`results/ramanujan/seed0_old` 则写为 `results/ramanujan/seed0_old_pernode.csv`）。

