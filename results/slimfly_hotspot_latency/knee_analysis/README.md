# Knee Analysis (Hotspot Injection Rate)

This folder stores standalone outputs for hotspot injection-rate knee analysis.

## Files

- `injection_knee_summary_full.csv`  
  Full table copied from `results/slimfly_hotspot_latency/injection_knee_summary.csv`.

- `injection_knee_summary_compact.csv`  
  Compact table for quick comparison and plotting.

- `injection_knee_minimal.csv`  
  Minimal table with only topology size and key knee injection-rate columns.

- `injection_knee_numeric.csv`  
  Numeric-only table (`N`, `knee_ppp_95pct_throughput`, `knee_ppp_latency_geometry`) for direct plotting/Excel.

- `plot_knee_ppp_vs_N.py`  
  Plot script for knee injection rate vs topology size `N`.

## Plot command

Run from anywhere:

```bash
python3 "/var/lib/data/zhang/network topology/topology-master-vc3_slimfly_valiant/topology-master-vc3/results/slimfly_hotspot_latency/knee_analysis/plot_knee_ppp_vs_N.py"
```

## Output figure

- `knee_ppp_vs_N.png`

## Notes

- `knee_ppp_95pct_throughput`: first injection-rate point where throughput reaches at least 95% of its peak.
- `knee_ppp_latency_geometry`: geometric knee on the latency-vs-offered curve.
- If a topology has only one injection-rate sample point, geometric knee is empty and status indicates insufficient points.
