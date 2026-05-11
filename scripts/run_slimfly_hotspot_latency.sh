#!/bin/bash

# Sweep Offered Traffic (ppp) and collect Latency vs Offered Traffic data under hotspot traffic.
# hotspot model in simulator (TRAFFIC_MODE=2):
#   - HOT_RATE_PERCENT% traffic -> fixed hotspot nodes
#   - (100-HOT_RATE_PERCENT)% traffic -> uniform random destinations
# Output layout:
#   results/slimfly_hotspot_latency/
#     <topology>_hotXX/
#       ppp_1000/ ... per-run artifacts ...
#       ppp_2000/ ...
#       ...
#       latency_vs_offered.csv
#       summary.txt

set -u

# Ensure we are at project root
cd "$(dirname "$0")/.." || exit 1

mkdir -p temp

# Prepare topology files in temp/
SRC_DIR="../../topology-master-vc3/temp"
if [ -d "$SRC_DIR" ]; then
    echo "Moving files from $SRC_DIR ..."
    mv -f "$SRC_DIR"/slimfly_*.txt temp/ 2>/dev/null
fi

cd temp || exit 1
for f in *_deg*.txt; do
    if [ -f "$f" ]; then
        NEW_NAME="${f/deg/D}"
        mv -f "$f" "$NEW_NAME"
    fi
done
find . -maxdepth 1 -type f ! -name "slimfly_N*_D*.txt" -delete
cd .. || exit 1

# ------------------ Config ------------------
TRAFFIC_MODE=2            # 2: hotspot
ROUTE_LUT_MODE=1          # baseline suite 1212: 1 (Tree-turn)
SEED=42
PACKETS_NUM=10            # small-data quick run: packets per selected flow event
ROOT_SELECT=2             # baseline suite 1212: 2 (Optimal root)
PATH_DIVERSITY=1          # baseline suite 1212: 1 (Non-minimal)
LOAD_BALANCE=2            # baseline suite 1212: 2 (Non-local congestion aware)
TRAFFIC_NUM=1000          # only used for mode -1, kept for CLI compatibility

# Hotspot parameters (read by route_sim_anytopo.c via environment variables)
HOT_RATE_PERCENT=20       # 20% -> hotspot, 80% -> uniform random
HOTSPOT_COUNT=1           # number of fixed hotspot destinations
HOTSPOT_BASE=0            # hotspot IDs: HOTSPOT_BASE..HOTSPOT_BASE+HOTSPOT_COUNT-1 (mod N)

# Offered traffic sweep (small-data quick run)
PPP_LIST=(200 500 1000)

# Topologies to process (建议先从小规模开始，跑通后再放开更大规模)
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

RESULTS_ROOT="results/slimfly_hotspot_latency"
mkdir -p "$RESULTS_ROOT"

echo "=== Hotspot Latency-vs-Offered Sweep ==="
echo "Profile: baseline-suite-1212 + hotspot"
echo "TRAFFIC_MODE=${TRAFFIC_MODE}, HOT_RATE_PERCENT=${HOT_RATE_PERCENT}%, HOTSPOT_COUNT=${HOTSPOT_COUNT}, HOTSPOT_BASE=${HOTSPOT_BASE}"
echo "ROUTE_LUT_MODE=${ROUTE_LUT_MODE}, PATH_DIVERSITY=${PATH_DIVERSITY}, LOAD_BALANCE=${LOAD_BALANCE}, PACKETS_NUM=${PACKETS_NUM}"

describe_config() {
    echo "Traffic: TRAFFIC_MODE=${TRAFFIC_MODE} (2=hotspot), packets_per_event=${PACKETS_NUM}"
    echo "Hotspot: HOT_RATE_PERCENT=${HOT_RATE_PERCENT}%, HOTSPOT_COUNT=${HOTSPOT_COUNT}, HOTSPOT_BASE=${HOTSPOT_BASE}"
    echo "Routing: ROUTE_LUT_MODE=${ROUTE_LUT_MODE}, PATH_DIVERSITY=${PATH_DIVERSITY}, LOAD_BALANCE=${LOAD_BALANCE}, ROOT_SELECT=${ROOT_SELECT}, SEED=${SEED}"
}

for FILE in "${FILES[@]}"; do
    if [ ! -f "$FILE" ]; then
        echo "Warning: missing topology $FILE, skip"
        continue
    fi

    BASENAME=$(basename "$FILE" .txt)
    TOPO_DIR="$RESULTS_ROOT/${BASENAME}_hot${HOT_RATE_PERCENT}"
    mkdir -p "$TOPO_DIR"

    CSV_FILE="$TOPO_DIR/latency_vs_offered.csv"
    echo "ppp,offered_traffic,avg_latency,total_cycle,throughput,total_packets,hot_rate_percent,hotspot_count,hotspot_base,topology" > "$CSV_FILE"

    AGG_SUMMARY_FILE="$TOPO_DIR/summary.txt"
    {
        echo "Hotspot latency sweep summary"
        echo "topology=${BASENAME}, traffic_mode=${TRAFFIC_MODE}, route_lut_mode=${ROUTE_LUT_MODE}, path_diversity=${PATH_DIVERSITY}, load_balance=${LOAD_BALANCE}, seed=${SEED}"
        echo "hot_rate_percent=${HOT_RATE_PERCENT}, hotspot_count=${HOTSPOT_COUNT}, hotspot_base=${HOTSPOT_BASE}, packets_num=${PACKETS_NUM}"
        echo ""
        echo "ppp,offered_traffic,avg_latency,total_cycle,throughput,total_packets"
    } > "$AGG_SUMMARY_FILE"

    for PPP in "${PPP_LIST[@]}"; do
        RUN_DIR="$TOPO_DIR/ppp_${PPP}"
        mkdir -p "$RUN_DIR"

        PATHLOG="$RUN_DIR/pathlog.txt"
        SUMMARY="$RUN_DIR/summary_lut.txt"
        SIM_LOG="$RUN_DIR/sim_log.txt"

        OFFERED=$(awk -v p=$PPP 'BEGIN{printf "%.4f", p/10000.0}')
        echo "[${BASENAME}] ppp=${PPP} (offered=${OFFERED})"

        export HOT_RATE_PERCENT HOTSPOT_COUNT HOTSPOT_BASE
        {
            echo "----------------------------------------------------------------"
            echo "Run: ${BASENAME} / ppp=${PPP} / offered=${OFFERED}"
            describe_config
            echo "Outputs: pathlog=${PATHLOG}, summary_lut=${SUMMARY}"
            echo "Command: ./bin/main $FILE $TRAFFIC_MODE $ROUTE_LUT_MODE $SEED $PPP $PACKETS_NUM $ROOT_SELECT $PATH_DIVERSITY $LOAD_BALANCE $PATHLOG $SUMMARY $TRAFFIC_NUM"
            echo "----------------------------------------------------------------"
            ./bin/main "$FILE" $TRAFFIC_MODE $ROUTE_LUT_MODE $SEED $PPP $PACKETS_NUM $ROOT_SELECT $PATH_DIVERSITY $LOAD_BALANCE "$PATHLOG" "$SUMMARY" $TRAFFIC_NUM
        } | tee "$SIM_LOG"

        conda run -n TOPO python scripts/analyze_pathlog.py "$PATHLOG" "$RUN_DIR" || echo "Skipping analyze_pathlog.py (conda env TOPO not available)"

        AVG_LAT=$(grep -m1 "Avg Packet Delivery Latency" "$SIM_LOG" | awk -F':' '{gsub(/^[ \t]+|[ \t]+$/, "", $2); print $2}')
        TOTAL_CYCLE=$(grep -m1 "Total cycle" "$SIM_LOG" | awk -F':' '{gsub(/^[ \t]+|[ \t]+$/, "", $2); print $2}')
        THROUGHPUT=$(grep -m1 "Throughput" "$SIM_LOG" | awk -F':' '{gsub(/^[ \t]+|[ \t]+$/, "", $2); split($2,a," "); print a[1]}')
        TOTAL_PACKETS=$(grep -m1 "Total Packets" "$SIM_LOG" | awk -F':' '{gsub(/^[ \t]+|[ \t]+$/, "", $2); print $2}')

        [ -z "${AVG_LAT}" ] && AVG_LAT="NA"
        [ -z "${TOTAL_CYCLE}" ] && TOTAL_CYCLE="NA"
        [ -z "${THROUGHPUT}" ] && THROUGHPUT="NA"
        [ -z "${TOTAL_PACKETS}" ] && TOTAL_PACKETS="NA"

        echo "${PPP},${OFFERED},${AVG_LAT},${TOTAL_CYCLE},${THROUGHPUT},${TOTAL_PACKETS},${HOT_RATE_PERCENT},${HOTSPOT_COUNT},${HOTSPOT_BASE},${BASENAME}" >> "$CSV_FILE"
        echo "${PPP},${OFFERED},${AVG_LAT},${TOTAL_CYCLE},${THROUGHPUT},${TOTAL_PACKETS}" >> "$AGG_SUMMARY_FILE"

        SUMMARY_FILE="$RUN_DIR/summary.txt"
        if [ -f "$SUMMARY_FILE" ]; then
            echo "" >> "$SUMMARY_FILE"
            echo "--- Simulation Results ---" >> "$SUMMARY_FILE"
            grep "Total Packets :" "$SIM_LOG" >> "$SUMMARY_FILE" || true
            grep "Total cycle :" "$SIM_LOG" >> "$SUMMARY_FILE" || true
            grep "Throughput :" "$SIM_LOG" >> "$SUMMARY_FILE" || true
            grep "Avg Packet Delivery Latency" "$SIM_LOG" >> "$SUMMARY_FILE" || true
            grep "Max Packet Delivery Latency" "$SIM_LOG" >> "$SUMMARY_FILE" || true
        fi
    done

    echo "Saved: $CSV_FILE"
    echo "Saved: $AGG_SUMMARY_FILE"
done

echo "Done."
