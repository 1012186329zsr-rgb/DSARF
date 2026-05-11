#!/usr/bin/env python3
"""Organize results under a base directory (default: results/ramanujan).

Actions:
 - Move files named like seed0_pernode.csv, seed0_summary.txt, seed0_heatmap.png into
   results/ramanujan/seed0/{pernode.csv,summary.txt,heatmap.png}
 - If files remain in a seed directory that are not one of the 3 standard names,
   move them into a timestamped archive directory inside the seed directory.
 - Safe by default: run with --dry-run to see actions; use --apply to execute.

Usage:
  python3 scripts/organize_results.py --base results/ramanujan --apply

"""
import argparse
import os
import re
import shutil
from datetime import datetime


STANDARD_NAMES = {'pernode.csv', 'summary.txt', 'heatmap.png'}


def find_seed_files(base):
    """Return list of files in base that match seedN_* pattern."""
    files = []
    for entry in os.listdir(base):
        p = os.path.join(base, entry)
        if os.path.isfile(p) and re.match(r'^seed\d+_', entry):
            files.append(entry)
    return files


def ensure_seed_dir(base, seed):
    d = os.path.join(base, seed)
    os.makedirs(d, exist_ok=True)
    return d


def standardize_name(filename):
    # filename like seed0_pernode.csv or seed0_no_div_pernode.csv
    m = re.match(r'^(seed\d+)_(?:.*_)?(pernode\.csv|summary\.txt|heatmap\.(?:png|jpg|jpeg))$', filename)
    if not m:
        return None
    seed = m.group(1)
    name = m.group(2)
    # normalize heatmap extension to png
    if name.lower().startswith('heatmap'):
        name = 'heatmap.png'
    return seed, name


def move_seed_prefixed_files(base, dry_run=True):
    actions = []
    files = find_seed_files(base)
    for f in files:
        std = standardize_name(f)
        if std:
            seed, name = std
            src = os.path.join(base, f)
            dst_dir = ensure_seed_dir(base, seed)
            dst = os.path.join(dst_dir, name)
            actions.append((src, dst))
        else:
            # move other seed-prefixed files into seed dir archive later
            m = re.match(r'^(seed\d+)_', f)
            if m:
                seed = m.group(1)
                src = os.path.join(base, f)
                dst_dir = ensure_seed_dir(base, seed)
                dst = os.path.join(dst_dir, f)
                actions.append((src, dst))
    if dry_run:
        return actions
    for src, dst in actions:
        print('MOVE', src, '->', dst)
        shutil.move(src, dst)
    return actions


def archive_nonstandard(base, dry_run=True):
    # For each seed dir, move any files not in STANDARD_NAMES into archive_TIMESTAMP
    actions = []
    for entry in os.listdir(base):
        seed_dir = os.path.join(base, entry)
        if not os.path.isdir(seed_dir):
            continue
        # skip archive dirs
        if re.match(r'^archive_\d{8}_\d{6}$', entry):
            continue
        to_archive = []
        for f in os.listdir(seed_dir):
            p = os.path.join(seed_dir, f)
            if os.path.isfile(p) and f not in STANDARD_NAMES:
                to_archive.append(f)
        if not to_archive:
            continue
        ts = datetime.now().strftime('%Y%m%d_%H%M%S')
        ad = os.path.join(seed_dir, f'archive_{ts}')
        # prepare actions
        for f in to_archive:
            src = os.path.join(seed_dir, f)
            dst = os.path.join(ad, f)
            actions.append((src, dst))
    if dry_run:
        return actions
    # perform moves grouped by archive dir creation
    created = set()
    for src, dst in actions:
        archive_dir = os.path.dirname(dst)
        if archive_dir not in created:
            os.makedirs(archive_dir, exist_ok=True)
            created.add(archive_dir)
        print('ARCHIVE', src, '->', dst)
        shutil.move(src, dst)
    return actions


def main():
    parser = argparse.ArgumentParser(description='Organize results directory by seed and archive nonstandard files')
    parser.add_argument('--base', '-b', default='results/ramanujan', help='Base results directory')
    parser.add_argument('--apply', action='store_true', help='Actually move files (dry-run default)')
    args = parser.parse_args()

    base = args.base
    if not os.path.isdir(base):
        print('Base directory does not exist:', base)
        return 1

    dry = not args.apply
    print('Dry run' if dry else 'Executing moves', 'on', base)

    actions = move_seed_prefixed_files(base, dry_run=dry)
    if actions:
        print('\nPlanned moves:')
        for s,d in actions:
            print(s, '->', d)
    else:
        print('\nNo seed-prefixed files to move.')

    arch = archive_nonstandard(base, dry_run=dry)
    if arch:
        print('\nPlanned archival moves:')
        for s,d in arch:
            print(s, '->', d)
    else:
        print('\nNo non-standard files to archive.')

    if dry:
        print('\nDry-run complete. Re-run with --apply to perform the moves.')
    else:
        print('\nDone.')

if __name__ == "__main__":
    raise SystemExit(main())
