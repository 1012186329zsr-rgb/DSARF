#!/usr/bin/env python3
"""
Simple Flask-based web UI to run the topology simulator (`bin/main`) remotely
and show output in the browser. This is intentionally small and self-contained
so it can be used without adding templates or static files.

Usage:
  - Install dependencies: `pip3 install -r requirements.txt`
  - Run: `python3 tools/web_gui.py`
  - Open browser to: http://localhost:5000

Security note: This server is not hardened — run on a trusted network or
bind to localhost only (default). Do NOT expose to the public internet.
"""
from __future__ import annotations

import os
import shlex
import subprocess
from pathlib import Path
from typing import Optional

try:
    from flask import Flask, request, render_template_string, redirect, url_for
except ModuleNotFoundError:
    print('\n依赖缺失: Flask 模块未安装。请运行：\n  pip3 install flask\n或在 Conda 下：\n  conda install -c conda-forge flask\n')
    raise SystemExit(1)

HERE = Path(__file__).resolve().parent
APP = Flask(__name__)

TEMPLATE = """
<!doctype html>
<meta charset="utf-8">
<title>拓扑仿真器 GUI</title>
<h2>拓扑仿真器（Topology Simulator）</h2>
<form method=post>
    <label>1) 拓扑文件（file_name）：</label><br>
    <select name=topo>
        {% for f in topo_files %}
            <option value="{{ f }}" {% if f==topo_default %}selected{% endif %}>{{ f }}</option>
        {% endfor %}
    </select>
    <input type=text name=topo_custom size=60 placeholder="或粘贴其它路径（可选）"><br><br>

    <label>2) 流量模式（traffic_mode）：</label><br>
    <select name=traffic_mode>
        <option value="0">0 - 均匀随机（uniform random）</option>
        <option value="-1">-1 - 从文件读取（file）</option>
    </select><br><br>

    <label>3) 路由 LUT 模式（route_lut_mode）：</label><br>
    <select name=route_lut_mode>
        <option value="0">0 - L-turn</option>
        <option value="1" selected>1 - Tree-turn</option>
        <option value="2">2 - Octo / new-turn</option>
    </select><br><br>

    <label>4) 随机种子（seed，整数，默认 0）：</label><br>
    <input name=seed size=20 value="0"><br><br>

    <label>5) ppp（概率，0..10000，默认 100，表示 100/10000）：</label><br>
    <input name=ppp size=10 value="100"><br><br>

    <label>6) packets_num（每对 src-dst 包数，默认 100）：</label><br>
    <input name=packets_num size=10 value="100"><br><br>

    <label>7) root_select（0-fix,1-random,2-optimal，默认 0）：</label><br>
    <select name=root_select>
        <option value="0" selected>0 - fix</option>
        <option value="1">1 - random</option>
        <option value="2">2 - optimal</option>
    </select><br><br>

    <label>8) path_diversity_mode（-1/0/1，默认 0）：</label><br>
    <select name=path_diversity_mode>
        <option value="-1">-1 - no diversity</option>
        <option value="0" selected>0 - minimal diversity</option>
        <option value="1">1 - non-minimal diversity</option>
    </select><br><br>

    <label>9) load_balance_mode（0/1/2，默认 1）：</label><br>
    <select name=load_balance_mode>
        <option value="0">0 - equal load</option>
        <option value="1" selected>1 - local congestion aware</option>
        <option value="2">2 - non-local congestion aware</option>
    </select><br><br>

    <label>10) 输出 path_log 文件名（只需输入文件名，例如 <code>my_pathlog.txt</code>，将会保存到项目 `results/` 目录，必填）：</label><br>
    <input name=out_filename size=60 value="111.txt" placeholder="只填文件名，例如: myrun.txt（将保存到 results/）"><br><br>

    <label>11) 流量文件（traffic_name，仅当 traffic_mode=-1 时必填）：</label><br>
    <input name=out_traffic size=80 value=""><br><br>

    <label>12) traffic_num（流量矩阵节点数，仅当 traffic_mode=-1 时填写）：</label><br>
    <input name=traffic_num size=10 value="80"><br><br>

    <label>VC 数（可选，会被添加为 `--vc N`）：</label><br>
    <input name=vc_count size=6 placeholder="例如 4"><br><br>

    <label>补充原始参数（raw_args，可选，直接追加到命令行末尾）：</label><br>
    <input name=raw_args size=120 value=""><br>

    <p/>
    <input type=submit value='运行 仿真'>
</form>

{% if error %}
<h3 style="color:darkred">错误</h3>
<pre>{{ error }}</pre>
{% endif %}

{% if running %}
<h3>运行输出</h3>
<pre>{{ output }}</pre>
{% endif %}

{% if out_files %}
<h3>生成的输出文件</h3>
<ul>
    {% for f in out_files %}
        <li>{{ f }}</li>
    {% endfor %}
</ul>
{% endif %}

"""


def find_simulator() -> Optional[Path]:
    # Prefer repo `bin/main`, fall back to `build/topology` or system path
    candidates = [HERE.parent / 'bin' / 'main', HERE.parent / 'build' / 'topology', Path('bin/main')]
    for c in candidates:
        if c.exists() and os.access(str(c), os.X_OK):
            return c
    return None


@APP.route('/', methods=['GET', 'POST'])
def index():
    sim = find_simulator()
    # list topo candidates from temp/ (txt files)
    temp_dir = HERE.parent / 'temp'
    topo_files = []
    if temp_dir.exists():
        for p in sorted(temp_dir.iterdir()):
            if p.is_file() and p.suffix in ('.txt', '.adj', '.adjlist'):
                topo_files.append(str(p))
    topo_default = topo_files[0] if topo_files else ''
    args_default = ''
    output = ''
    running = False
    out_files = []
    error = ''

    if request.method == 'POST':
        # gather fields
        topo_sel = request.form.get('topo', '').strip()
        topo_custom = request.form.get('topo_custom', '').strip()
        traffic_mode = request.form.get('traffic_mode', '0').strip()
        route_lut_mode = request.form.get('route_lut_mode', '1').strip()
        seed = request.form.get('seed', '0').strip()
        ppp = request.form.get('ppp', '100').strip()
        packets_num = request.form.get('packets_num', '100').strip()
        root_select = request.form.get('root_select', '0').strip()
        path_diversity_mode = request.form.get('path_diversity_mode', '0').strip()
        load_balance_mode = request.form.get('load_balance_mode', '1').strip()
        out_filename = request.form.get('out_filename', '').strip()
        out_traffic = request.form.get('out_traffic', '').strip()
        traffic_num = request.form.get('traffic_num', '').strip()
        vc_count = request.form.get('vc_count', '').strip()
        raw_args = request.form.get('raw_args', '').strip()

        # decide topology path
        topo = topo_custom if topo_custom else topo_sel

        # prepare results path for path_log: only accept a filename, write into `results/`
        results_dir = (HERE.parent / 'results')
        try:
            results_dir.mkdir(parents=True, exist_ok=True)
        except Exception:
            pass

        # validate filename (no path separators, not empty)
        out_pathlog = ''
        if out_filename:
            if '/' in out_filename or '\\' in out_filename or out_filename.startswith('..'):
                out_filename = ''
            else:
                # ensure extension
                if not Path(out_filename).suffix:
                    out_filename = out_filename + '.txt'
                out_pathlog = str((results_dir / out_filename).resolve())

        # basic validation
        if not sim:
            error = '未找到可执行仿真程序（尝试查找 bin/main）。'
        elif not topo:
            error = '请指定拓扑文件（file_name）.'
        elif not out_pathlog:
            error = '请指定输出 path_log 文件名（将保存到 results/ 目录）.'
        elif traffic_mode == '-1' and (not out_traffic or not traffic_num):
            error = '选择从文件读取流量模式 (traffic_mode=-1) 时，必须填写 流量文件 (traffic_name) 和 traffic_num.'
        else:
            cmd = [str(sim)]
            cmd.append(topo)
            # append positional args in the expected order
            cmd.append(traffic_mode)
            cmd.append(route_lut_mode)
            cmd.append(seed)
            cmd.append(ppp)
            cmd.append(packets_num)
            cmd.append(root_select)
            cmd.append(path_diversity_mode)
            cmd.append(load_balance_mode)
            cmd.append(out_pathlog)
            # traffic_name and traffic_num
            cmd.append(out_traffic if out_traffic else "")
            cmd.append(traffic_num if traffic_num else "0")
            # raw args (appended)
            if raw_args:
                cmd += shlex.split(raw_args)
            # vc override
            if vc_count:
                cmd += ["--vc", vc_count]

            running = True
            try:
                proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=864000)
                output = proc.stdout.decode(errors='replace')
            except subprocess.TimeoutExpired:
                output = '错误: 仿真超时（1 小时限制）'
            except Exception as e:
                output = f'错误: {type(e).__name__}: {e}'

            # 列出生成的输出文件（只列出已知的、实际存在的文件）
            if out_pathlog:
                p = Path(out_pathlog)
                if p.exists():
                    out_files.append(str(p.resolve()))
            if out_traffic:
                p2 = Path(out_traffic)
                if p2.exists():
                    out_files.append(str(p2.resolve()))

    return render_template_string(TEMPLATE, topo_default=topo_default, topo_files=topo_files, args_default=args_default,
                                  output=output, running=running, out_files=out_files, error=error)

    return render_template_string(TEMPLATE, topo_default=topo_default, args_default=args_default,
                                  output=output, running=running, out_files=out_files)


@APP.route('/files/<path:fname>')
def serve_file(fname: str):
    # Serve files under workspace root only
    ws_root = (HERE.parent).resolve()
    p = (Path(fname)).resolve()
    try:
        if ws_root in p.parents or p == ws_root:
            return APP.send_static_file(str(p))
    except Exception:
        pass
    return 'File not found or outside workspace', 404


def main():
    print('Starting web GUI on http://127.0.0.1:5001')
    APP.run(host='127.0.0.1', port=5001, debug=False)


if __name__ == '__main__':
    main()
