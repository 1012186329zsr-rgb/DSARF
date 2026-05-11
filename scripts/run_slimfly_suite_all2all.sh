#!/bin/bash

# All-to-all run (TRAFFIC_MODE=1) with results isolated from baseline.
# Keeps the original scripts/run_slimfly_suite.sh untouched.

# -----------------------------
# 备注：所有选项说明（你可以在下面直接改数值）
# -----------------------------
# TRAFFIC_MODE:
#   -1: 从 traffic matrix 文件读取（CSV，N×N）
#    0: Uniform random（由 ppp 控制注入概率）
#    1: All-to-all（所有 i!=j 都发送，每对 (src,dst) 发送 packets_num 个包）
#
# ROUTE_LUT_MODE (Routing Algorithm):
#    0: L-turn
#    1: Tree-turn
#    2: Octo-turn
#    3: SlimFly MIN
#    4: Weighted Constrained (Hop<=3)  （本仓库中该模式可能使用 Source Routing 来固定路径）
#   5: Up/Down (deadlock-free turn constraint based on a BFS tree)
#
# ROOT_SELECT（仅对 Tree-turn 有意义，其他模式会被忽略/不影响结果）:
#    0: Fixed
#    1: Random
#    2: Optimal
#
# PATH_DIVERSITY:
#   -1: No diversity
#    0: Minimal
#    1: Non-minimal
#    2: Valiant (2-phase)
#
# LOAD_BALANCE:
#    0: Equal load（不做拥塞感知，仅等价均分/固定选择）
#    1: Local congestion aware（看本地 credit/拥塞）
#    2: Non-local congestion aware（使用非本地拥塞信息）
#
# CLI format (bin/main 参数顺序):
#   ./bin/main <topo> <traffic_mode> <route_lut_mode> <seed> <ppp> <packets_num> <root_select>
#            <path_diversity> <load_balance> <pathlog> <summary_lut> <traffic_num>
#
# sim_log.txt 表述约定：
# - 当 TRAFFIC_MODE=1 (all-to-all) 时，ppp/traffic_num 仅为占位参数（程序仍会打印，但不影响注入）。
# - ROOT_SELECT 只有在 ROUTE_LUT_MODE=1 (Tree-turn) 时才真正生效。

# Ensure we are in the project root
cd "$(dirname "$0")/.."

# Ensure temp directory exists
mkdir -p temp

# 1. Move files from original location to temp (Overwrite if exists -f)
SRC_DIR="../../topology-master-vc3/temp"
if [ -d "$SRC_DIR" ]; then
    echo "Moving files from $SRC_DIR..."
    mv -f "$SRC_DIR"/slimfly_*.txt temp/ 2>/dev/null
fi

# Process files in temp
cd temp || exit

# 2. Rename _deg to _D (Overwrite if exists -f)
for f in *_deg*.txt; do
    if [ -f "$f" ]; then
        NEW_NAME="${f/deg/D}"
        echo "Renaming $f to $NEW_NAME"
        mv -f "$f" "$NEW_NAME"
    fi
done

# 3. Clean up non-SlimFly files (Keep only slimfly_N*_D*.txt)
echo "Cleaning up non-SlimFly files..."
find . -maxdepth 1 -type f ! -name "slimfly_N*_D*.txt" -delete

# Return to project root
cd ..
echo "Topology files prepared in temp/"
ls -1 temp/

# Configuration
TRAFFIC_MODE=1       # Traffic Mode (1: All-to-All, 0: Uniform Random, -1: From File)
ROUTE_LUT_MODE=4   # Routing Algorithm (0: L-turn, 1: Tree-turn, 2: Octo-turn, 3: SlimFly MIN, 4: Weighted Constrained, 5: Up/Down)
SEED=0               # Random Seed (0: Random based on time/pid)
PPP=1000             # Packet Injection Rate (Probability Per 10000 cycles, e.g., 1000 = 10%)
PACKETS_NUM=1000        # Number of packets per (src,dst) pair in all-to-all
ROOT_SELECT=0       # Root Selection for Tree-turn (0: Fixed, 1: Random, 2: Optimal)
PATH_DIVERSITY=2    # Path Diversity (-1: No diversity, 0: Minimal, 1: Non-minimal, 2: Valiant)
LOAD_BALANCE=1       # Load Balancing (0: Equal, 1: Local Congestion Aware, 2: Non-local Congestion Aware)
TRAFFIC_NUM=1000     # Number of nodes for traffic file (only used if TRAFFIC_MODE=-1)


# Output directory (separate from baseline)
RESULTS_ROOT="results/slimfly_all2all"
mkdir -p "$RESULTS_ROOT"

# Files to process (uncomment the ones you want to run)
# Tip: you can uncomment all of them to run up to N=1682.
FILES=(
    "temp/slimfly_N50_D7.txt"
    "temp/slimfly_N98_D11.txt"
    "temp/slimfly_N242_D17.txt"
    "temp/slimfly_N338_D19.txt"
    "temp/slimfly_N578_D25.txt"
    "temp/slimfly_N722_D29.txt"
    "temp/slimfly_N1058_D35.txt"
    "temp/slimfly_N1682_D43.txt"
)

if [ ${#FILES[@]} -eq 0 ]; then
    echo "No topology files selected. Edit this script and uncomment entries in FILES=(...)."
    exit 1
fi

describe_config() {
    echo "Traffic: TRAFFIC_MODE=${TRAFFIC_MODE} (all-to-all if =1), packets_per_pair=${PACKETS_NUM}"
    if [ "$TRAFFIC_MODE" -eq 1 ]; then
        echo "Note: ppp=${PPP} and traffic_num=${TRAFFIC_NUM} are ignored in all-to-all mode (kept for CLI compatibility)."
    else
        echo "Note: ppp=${PPP} may affect injection when traffic_mode!=1."
    fi
    echo "Routing: ROUTE_LUT_MODE=${ROUTE_LUT_MODE}, PATH_DIVERSITY=${PATH_DIVERSITY}, LOAD_BALANCE=${LOAD_BALANCE}"
    if [ "$ROUTE_LUT_MODE" -eq 1 ]; then
        echo "Tree-turn: ROOT_SELECT=${ROOT_SELECT} (0=fixed,1=random,2=optimal)"
    else
        echo "Note: ROOT_SELECT=${ROOT_SELECT} only matters for Tree-turn (route_lut_mode=1)."
    fi
    echo "Seed: ${SEED}"
}

traffic_mode_name() {
    case "$TRAFFIC_MODE" in
        -1) echo "from-file" ;;
        0) echo "uniform-random" ;;
        1) echo "all-to-all" ;;
        *) echo "unknown(${TRAFFIC_MODE})" ;;
    esac
}

route_lut_mode_name() {
    case "$ROUTE_LUT_MODE" in
        0) echo "L-turn" ;;
        1) echo "Tree-turn" ;;
        2) echo "Octo-turn" ;;
        3) echo "SlimFly-MIN" ;;
        4) echo "Weighted-Constrained" ;;
        5) echo "Up/Down" ;;
        *) echo "unknown(${ROUTE_LUT_MODE})" ;;
    esac
}

path_diversity_name() {
    case "$PATH_DIVERSITY" in
        -1) echo "no-diversity" ;;
        0) echo "minimal" ;;
        1) echo "non-minimal" ;;
        2) echo "valiant" ;;
        3) echo "ugal" ;;
        *) echo "unknown(${PATH_DIVERSITY})" ;;
    esac
}

load_balance_name() {
    case "$LOAD_BALANCE" in
        0) echo "equal" ;;
        1) echo "local" ;;
        2) echo "non-local" ;;
        *) echo "unknown(${LOAD_BALANCE})" ;;
    esac
}

print_config_summary() {
    local traffic_name route_name div_name lb_name
    traffic_name=$(traffic_mode_name)
    route_name=$(route_lut_mode_name)
    div_name=$(path_diversity_name)
    lb_name=$(load_balance_name)

    if [ "$ROUTE_LUT_MODE" -eq 1 ]; then
        echo "Configuration: traffic=${traffic_name}, route=${route_name}(root_select=${ROOT_SELECT}), diversity=${div_name}, lb=${lb_name}, packets_per_pair=${PACKETS_NUM}, results=${RESULTS_ROOT}"
    else
        echo "Configuration: traffic=${traffic_name}, route=${route_name}, diversity=${div_name}, lb=${lb_name}, packets_per_pair=${PACKETS_NUM}, results=${RESULTS_ROOT}"
    fi
}

echo "Starting Slimfly Simulation Suite (All-to-all)..."
print_config_summary

for FILE in "${FILES[@]}"; do
    if [ -f "$FILE" ]; then
        BASENAME=$(basename "$FILE" .txt)
        RESULT_DIR="$RESULTS_ROOT/${BASENAME}_all2all"
        mkdir -p "$RESULT_DIR"

        echo "----------------------------------------------------------------"
        echo "Running simulation for $BASENAME..."

        PATHLOG="${RESULT_DIR}/pathlog.txt"
        SUMMARY="${RESULT_DIR}/summary_lut.txt"

        SIM_LOG="${RESULT_DIR}/sim_log.txt"
        {
            echo "----------------------------------------------------------------"
            echo "Run: ${BASENAME}"
            describe_config
            echo "Outputs: pathlog=${PATHLOG}, summary_lut=${SUMMARY}"
            echo "Command: ./bin/main $FILE $TRAFFIC_MODE $ROUTE_LUT_MODE $SEED $PPP $PACKETS_NUM $ROOT_SELECT $PATH_DIVERSITY $LOAD_BALANCE $PATHLOG $SUMMARY $TRAFFIC_NUM"
            echo "----------------------------------------------------------------"
            ./bin/main "$FILE" $TRAFFIC_MODE $ROUTE_LUT_MODE $SEED $PPP $PACKETS_NUM $ROOT_SELECT $PATH_DIVERSITY $LOAD_BALANCE "$PATHLOG" "$SUMMARY" $TRAFFIC_NUM
        } | tee "$SIM_LOG"

        conda run -n TOPO python scripts/analyze_pathlog.py "$PATHLOG" "$RESULT_DIR" || echo "Skipping analyze_pathlog.py (conda env TOPO not available)"

        SUMMARY_FILE="${RESULT_DIR}/summary.txt"
        if [ -f "$SUMMARY_FILE" ]; then
            echo "" >> "$SUMMARY_FILE"
            echo "--- Simulation Results ---" >> "$SUMMARY_FILE"
            grep "Total Packets :" "$SIM_LOG" >> "$SUMMARY_FILE" || true
            grep "Total cycle :" "$SIM_LOG" >> "$SUMMARY_FILE" || true
            grep "Throughput :" "$SIM_LOG" >> "$SUMMARY_FILE" || true
            grep "Avg Packet Delivery Latency" "$SIM_LOG" >> "$SUMMARY_FILE" || true
            grep "Max Packet Delivery Latency" "$SIM_LOG" >> "$SUMMARY_FILE" || true
        fi

        echo "Finished $BASENAME"
    else
        echo "Warning: File $FILE not found, skipping."
    fi
done

echo "All all-to-all simulations completed."