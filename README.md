# DSARF Network Simulator

A high-performance network topology simulator for interconnection network research, featuring adaptive routing algorithms and support for Slimfly topologies.

## Quick Start

### Build

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target all
```

This creates the executable `bin/main` at the repository root.

### Usage

```bash
bin/main <topology_file> <routing_algorithm> <traffic_pattern> <num_seed> \
           <packet_num> <print_interval> <injection_rate_start> <injection_rate_end> \
           <injection_rate_step> <pathlog_file> <traffic_file> <vc_num>
```

**Parameters:**
- `topology_file`: Path to topology description file (format: `num_nodes,degree` on first line, followed by adjacency lists)
- `routing_algorithm`: 0=Valiant, 1=Minimal, 2=Adaptive, etc.
- `traffic_pattern`: 0=Uniform, 1=Hotspot, 2=All-to-All, etc.
- `num_seed`: Number of random seeds to run
- `packet_num`: Number of packets per simulation
- `print_interval`: Output frequency
- `injection_rate_start/end/step`: Traffic injection rate sweep (packets per cycle)
- `pathlog_file`: Output file for path logs
- `traffic_file`: Output file for traffic statistics (CSV)
- `vc_num`: Number of virtual channels (runtime configurable)

### Example

```bash
# Run simulation on a small topology with 8 virtual channels
bin/main examples/topology.txt 1 0 3 10000 1000 0.001 0.01 0.001 \
       output/pathlog.txt output/traffic.csv 8
```

## Output Files

The simulator generates two output files:

1. **Path log** (`pathlog_file`): Detailed per-packet routing information
2. **Traffic CSV** (`traffic_file`): Per-node statistics including latency, throughput, and congestion metrics

## Supported Topologies

- Slimfly networks
- Dragonfly networks
- Generic arbitrary topologies (via adjacency list input)

## Requirements

- GCC or Clang compiler
- CMake 3.10+
- OpenMP (optional, for parallelization)

## Citation

If you use this simulator in your research, please cite our paper.
