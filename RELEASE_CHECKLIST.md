# Release Checklist (GitHub)

This checklist is for publishing `topology-master-vc3` safely without losing research data.

## 1. Keep vs Ignore

Keep in repository:
- `src/`, `include/`, `scripts/`, `tools/`, `draw/`
- `CMakeLists.txt`, `Makefile`, `requirements.txt`
- `README.md`, `README_DETAILED.md`, `SUMMARY.md`, `REPO_SUMMARY.md`
- Curated result summaries you want public

Do not version generated artifacts:
- `build/`, `bin/`, `obj/`, `temp/`
- Runtime logs and temporary outputs
- Large, reproducible raw path logs unless required for a paper artifact

## 2. Pre-publish checks

Run from repo root:

```bash
git status --short
```

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

(Optionally) basic run smoke test with a tiny topology.

## 3. Results policy recommendation

For GitHub readability and clone speed:
- Keep only final/curated tables in `results/`
- Move full raw outputs to a separate data archive (Zenodo/Drive/Institution storage)
- In README, link to external archive and include reproduction script arguments

## 4. Before first public push

- Confirm `.gitignore` covers generated files
- Remove accidental local commands from docs
- Verify no credentials or machine-local paths are present
- Tag first public baseline after push (example: `v1.0.0`)

## 5. Suggested branch flow

- `main`: stable publish branch
- `exp/*`: experiment branches
- `archive/*`: historical backups or one-time migrations
