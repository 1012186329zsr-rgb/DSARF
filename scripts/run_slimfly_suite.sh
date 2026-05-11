#!/bin/bash

# --- File Preparation Start ---
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
# --- File Preparation End ---

# Configuration
TRAFFIC_MODE=0       # Traffic Mode (1: All-to-All, 0: Uniform Random, -1: From File)
ROUTE_LUT_MODE=4     # Routing Algorithm (0: L-turn, 1: Tree-turn, 2: Octo-turn, 3: SlimFly MIN, 4: Weighted Constrained, 5: Up/Down)
SEED=0               # Random Seed (0: Random based on time/pid)
PPP=1000             # Packet Injection Rate (Probability Per 10000 cycles, e.g., 1000 = 10%)
PACKETS_NUM=1000         # Number of packets per (src,dst) pair in all-to-all (keep small for first trial)
ROOT_SELECT=0     # Root Selection for Tree-turn (0: Fixed, 1: Random, 2: Optimal)
PATH_DIVERSITY=2    # Path Diversity (-1: No diversity, 0: Minimal, 1: Non-minimal, 2: Valiant)
LOAD_BALANCE=2       # Load Balancing (0: Equal, 1: Local Congestion Aware, 2: Non-local Congestion Aware)
TRAFFIC_NUM=1000     # Number of nodes for traffic file (only used if TRAFFIC_MODE=-1)

# Output directory
mkdir -p results/slimfly_mine_new

# Files to process
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

echo "Starting Slimfly Simulation Suite..."
echo "Configuration: TRAFFIC_MODE=${TRAFFIC_MODE}, ROUTE_LUT_MODE=${ROUTE_LUT_MODE}, PATH_DIVERSITY=${PATH_DIVERSITY}, LOAD_BALANCE=${LOAD_BALANCE}, PACKETS_NUM=${PACKETS_NUM}, ROOT_SELECT=${ROOT_SELECT}, PPP=${PPP}, SEED=${SEED}"

for FILE in "${FILES[@]}"; do
    if [ -f "$FILE" ]; then
        BASENAME=$(basename "$FILE" .txt)
        RESULT_DIR="results/slimfly_mine_new/${BASENAME}_slimfly_min"
        mkdir -p "$RESULT_DIR"

        echo "----------------------------------------------------------------"
        echo "Running simulation for $BASENAME..."
        
        PATHLOG="${RESULT_DIR}/pathlog.txt"
        SUMMARY="${RESULT_DIR}/summary_lut.txt"
        
        # Command format:
        # ./bin/main <topo> <traffic> <lut> <seed> <ppp> <pkts> <root> <div> <lb> <pathlog> <summary> <traffic_num>
        
        SIM_LOG="${RESULT_DIR}/sim_log.txt"
        ./bin/main "$FILE" $TRAFFIC_MODE $ROUTE_LUT_MODE $SEED $PPP $PACKETS_NUM $ROOT_SELECT $PATH_DIVERSITY $LOAD_BALANCE "$PATHLOG" "$SUMMARY" $TRAFFIC_NUM | tee "$SIM_LOG"
        
        # Generate analysis and plots
        conda run -n TOPO python scripts/analyze_pathlog.py "$PATHLOG" "$RESULT_DIR"
        
        # Append simulation results to summary.txt
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

echo "All simulations completed."
