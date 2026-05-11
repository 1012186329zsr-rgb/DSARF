#!/usr/bin/env python3
"""
run_experiments.py — 初稿：运行实验并整理输出日志/结果

功能场景（初稿）：
- 从 JSON 配置读取一组实验（每项指定拓扑、参数和可选的分析步骤）
- 校验拓扑文件格式（第一行应为: num_nodes,degree）
- 根据可配置的参数模板构造命令并调用二进制程序
- 将 stdout/stderr 和生成的 pathlog/traffic 文件收集到 results/<run_name>_<ts>/
- 调用仓库里已有的分析脚本（如果配置中指定）来生成汇总
- 记录每次运行的元数据为 JSON

使用示例：
    python3 scripts/run_experiments.py --config scripts/experiment_config.json
    python3 scripts/run_experiments.py --config scripts/experiment_config.json --dry-run

注意：这是初稿，欢迎你指出需要调整的参数、命名或额外步骤。
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import time
from datetime import datetime


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def read_json(path):
    with open(path, 'r', encoding='utf-8') as f:
        return json.load(f)


def write_json(path, obj):
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(obj, f, indent=2, ensure_ascii=False)


def validate_topology(topo_path):
    # 简单校验：文件存在、首行包含两个逗号分隔的整数或逗号分隔的两个数字
    if not os.path.isfile(topo_path):
        return False, 'topology file not found'
    try:
        with open(topo_path, 'r', encoding='utf-8') as f:
            first = f.readline().strip()
        parts = first.replace(' ', '').split(',')
        if len(parts) < 2:
            return False, f'first line has <2 comma-separated values: "{first}"'
        int(parts[0])
        int(parts[1])
        return True, ''
    except Exception as e:
        return False, str(e)


def format_args(template, subs):
    # template: list of tokens or single string with placeholders like {topo} {ppp}
    if isinstance(template, list):
        return [t.format(**subs) for t in template]
    else:
        return template.format(**subs).split()


def run_single(exe, args, workdir, stdout_path, stderr_path, dry_run=False):
    cmd = [exe] + args
    print('Running:', ' '.join(cmd))
    if dry_run:
        print('(dry-run) would run in', workdir)
        return 0
    with open(stdout_path, 'wb') as out_f, open(stderr_path, 'wb') as err_f:
        proc = subprocess.run(cmd, stdout=out_f, stderr=err_f, cwd=workdir)
        return proc.returncode


def call_analysis(script_path, inputs, outdir, dry_run=False):
    # 如果脚本存在则调用：python3 script_path [inputs...]
    if not os.path.isfile(script_path):
        print(f'Analysis script not found: {script_path} — skipping')
        return None
    cmd = [sys.executable, script_path] + inputs
    print('Calling analysis:', ' '.join(cmd))
    if dry_run:
        return 0
    try:
        proc = subprocess.run(cmd, cwd=outdir)
        return proc.returncode
    except Exception as e:
        print('Analysis call failed:', e)
        return -1


def main():
    parser = argparse.ArgumentParser(description='Run experiment runs and collect logs (draft)')
    parser.add_argument('--config', '-c', required=True, help='JSON config file with runs list')
    parser.add_argument('--outroot', '-o', default='results', help='root directory to store results')
    parser.add_argument('--dry-run', action='store_true', help='only print actions without executing')
    parser.add_argument('--keep-temp', action='store_true', help='do not delete intermediate files in working dir')
    args = parser.parse_args()

    cfg = read_json(args.config)
    runs = cfg.get('runs', [])
    if not runs:
        print('Config has no runs to execute. Edit', args.config)
        return

    ensure_dir(args.outroot)
    summary = {
        'started_at': datetime.utcnow().isoformat() + 'Z',
        'runs': []
    }

    for r in runs:
        name = r.get('name') or f"run_{int(time.time())}"
        ts = datetime.now().strftime('%Y%m%d_%H%M%S')
        outdir = os.path.join(args.outroot, f'{name}_{ts}')
        ensure_dir(outdir)

        exe = r.get('binary') or cfg.get('default_binary') or 'bin/main'
        topo = r.get('topology')
        if not topo:
            print('Run', name, 'missing topology — skipping')
            continue

        ok, reason = validate_topology(topo)
        if not ok:
            print(f'Topology validation failed for {topo}:', reason)
            # still continue depending on config choice
            if not r.get('ignore_topo_errors', False):
                print('Skipping run', name)
                continue

        # substitution values available for templates
        subs = {
            'topo': os.path.abspath(topo),
            'ppp': str(r.get('ppp', cfg.get('default_ppp', 1000))),
            'seed': str(r.get('seed', 0)),
            'packet_num': str(r.get('packet_num', cfg.get('default_packet_num', 1))),
            'vc': str(r.get('vc', cfg.get('default_vc', 8))),
        }

        # pathlog and traffic file locations (inside outdir)
        pathlog = os.path.join(outdir, r.get('pathlog_name', 'pathlog.txt'))
        traffic = os.path.join(outdir, r.get('traffic_name', 'traffic.csv'))
        subs['pathlog'] = pathlog
        subs['traffic'] = traffic

        # argument template: default is a positional template (example from README)
        arg_template = r.get('arg_template') or cfg.get('arg_template') or '{topo} 0 0 0 {ppp} {seed} 0 0 0 {pathlog} {traffic} {vc}'
        args_list = format_args(arg_template, subs)

        stdout_path = os.path.join(outdir, 'run_stdout.log')
        stderr_path = os.path.join(outdir, 'run_stderr.log')

        meta = {
            'name': name,
            'outdir': outdir,
            'exe': exe,
            'args': args_list,
            'started_at': datetime.utcnow().isoformat() + 'Z'
        }

        # run the program
        rc = run_single(exe, args_list, '.', stdout_path, stderr_path, dry_run=args.dry_run)
        meta['return_code'] = rc
        meta['finished_at'] = datetime.utcnow().isoformat() + 'Z'

        # post-process: compress pathlog/traffic if exist
        for fname in [pathlog, traffic]:
            if os.path.exists(fname):
                try:
                    shutil.copy(fname, outdir)
                except Exception:
                    pass

        # call analysis steps (if provided)
        analyses = r.get('analyses', cfg.get('default_analyses', []))
        analysis_results = []
        for a in analyses:
            script = a.get('script')
            inputs = [os.path.join(outdir, inp.format(**subs)) if '{' in inp else inp for inp in a.get('inputs', [])]
            rc2 = call_analysis(script, inputs, outdir, dry_run=args.dry_run)
            analysis_results.append({'script': script, 'rc': rc2})

        meta['analyses'] = analysis_results

        # save metadata
        write_json(os.path.join(outdir, 'run_metadata.json'), meta)

        summary['runs'].append(meta)

        if not args.keep_temp:
            # (初稿) 不移除生成的 files — 更复杂的清理可选
            pass

    summary['finished_at'] = datetime.utcnow().isoformat() + 'Z'
    write_json(os.path.join(args.outroot, 'batch_summary.json'), summary)
    print('All done. Summary written to', os.path.join(args.outroot, 'batch_summary.json'))


if __name__ == '__main__':
    main()
