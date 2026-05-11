topology-master-vc3 — Detailed README
=====================================

Overview
--------
This repository is a working copy of `topology-master` that has been modified
to support runtime-configurable virtual channels (VCs). The changes are kept
in a separate directory `topology-master-vc3` so the original source
remains intact.

Goals of this fork
------------------
- Allow experiments to change the number of VCs without editing many source
  files or recompiling different macro values.
- Replace fixed-size arrays indexed by a compile-time macro with dynamic
  allocations based on a runtime `vc_num` per router.
- Keep backward compatibility where reasonable (the compile-time macro
  `ROUTE_SIM_ANYTOPO_VC` remains defined as a default value).

Key files and locations
-----------------------
- `include/route_sim_anytopo.h` — type & macro declarations. Declares
  `extern int ROUTE_SIM_ANYTOPO_VC_RUNTIME;`.
- `src/globals.c` — new file that defines `ROUTE_SIM_ANYTOPO_VC_RUNTIME`.
- `src/route_sim_anytopo.c`, `src/path_with_turn_forbidden.c`, etc. — core
  code modified to allocate per-router structures using `R->vc_num`.
- `bin/main` — the compiled executable (after running CMake/Make).
- `scripts/` — placeholder where we can add run/test scripts (development
  scripts were temporarily used in `/tmp`).
- `results/` — place to store CSV/summary outputs if you want them in repo.

Build instructions
------------------
Standard CMake-based build:

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target all
```

This creates `bin/main` at the repository root.

Running and examples
--------------------
Program arguments are the same as original `topology-master` except the VC
number is read from `ROUTE_SIM_ANYTOPO_VC_RUNTIME` at runtime. (If you prefer
CLI parsing, see the next section.)

A minimal quick run (uses a small temp topology used for tests):

```bash
bin/main /tmp/small_topo3.txt 0 0 0 1000 1 0 0 0 /tmp/pathlog.txt /tmp/traffic.csv 8
```

Example batch scripts used during development (temporary location `/tmp`):
- `/tmp/run_tests_clean.sh` — small sweep (ppp=1000,3000,5000 × seeds 0..2)
- `/tmp/run_extended.sh` — extended sweep (ppp=1000,3000,5000,7000,9000 × seeds 0..4; packet_num=10)

Where experiments write outputs
--------------------------------
- Path logs: `/tmp/pathlog_<ppp>_<seed>.txt`
- Traffic CSV: `/tmp/traffic_<ppp>_<seed>.csv`
- Aggregated summaries: `/tmp/test_runs_summary.txt`, `/tmp/test_runs_extended_summary.txt`

How to add a CLI `--vc` flag (recommended small change)
-----------------------------------------------------
1. Edit `src/topology.c` and parse an additional optional argument for VC
   count (or use `getopt_long`).
2. Validate the value (e.g., 1 ≤ vc ≤ 64).
3. Set `ROUTE_SIM_ANYTOPO_VC_RUNTIME = parsed_value;` before constructing
   RouterList structures.

If you want, I can implement this change (small patch): parse `--vc N` or
an extra positional argument and set the global runtime variable.

Notes on correctness and testing
--------------------------------
- The dynamic allocation changes were tested on small topologies (4-node,
  8-node ring) and the executable built successfully in `topology-master-vc3`.
- Watch out for input topology formatting: the first line must be `num_nodes,degree` and adjacency lists must be comma separated.

Troubleshooting
---------------
- If the program prints: "ERROR: failed to read num_nodes,degree" — check the topology file's first line format.
- If you see segfaults after changing arrays, it's likely a missing allocation or incorrect indices caused by an incomplete conversion from compile-time arrays to dynamic arrays. Search for remaining direct uses of `ROUTE_SIM_ANYTOPO_VC` that weren't replaced.

Suggested next steps
--------------------
- Move the temporary test scripts from `/tmp` into `scripts/` in the repo and add a `results/` directory to commit CSV outputs.
- Add a CLI `--vc` flag so experiments can be launched with arbitrary VC counts.
- Convert the test harness to write a CSV summary into `results/`.

Repository structure for GitHub publish
---------------------------------------
- Keep as source-of-truth: `src/`, `include/`, `scripts/`, `tools/`, `draw/`, `CMakeLists.txt`, `Makefile`, `README*.md`.
- Treat as generated and not for versioning: `build/`, `bin/`, `obj/`, `temp/` and runtime logs.
- Keep only curated experiment artifacts in `results/` (summary CSV/TXT you explicitly want to publish).

Patch / merge guidance
----------------------
If you want to merge these changes back into the original `topology-master`, I can prepare a patch or git diff. The change touches header layout and many core files; review is recommended.

Contact
-------
Tell me if you want the CLI flag added, scripts moved into repo, or a patch prepared.