#!/usr/bin/env python3
"""
Small Tkinter GUI to run the topology simulator without typing long CLI commands.
Creates a simple form for the most-used parameters and shows live stdout/stderr.
"""
import os
import shlex
import subprocess
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext

# Default executable candidates
DEFAULT_EXES = ["./bin/main", "build/topology", "./topology", "bin/main"]

class SimulatorGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Topology Simulator Runner")
        self.geometry("900x700")

        self._build_ui()
        self.proc = None

    def _build_ui(self):
        frm = tk.Frame(self)
        frm.pack(fill=tk.X, padx=8, pady=6)

        # Executable
        tk.Label(frm, text="Simulator executable:").grid(row=0, column=0, sticky=tk.W)
        self.exe_var = tk.StringVar(value=self._find_default_exe())
        tk.Entry(frm, textvariable=self.exe_var, width=48).grid(row=0, column=1, sticky=tk.W)
        tk.Button(frm, text="Browse", command=self._pick_executable).grid(row=0, column=2, padx=6)

        # Topology file
        tk.Label(frm, text="Topology file:").grid(row=1, column=0, sticky=tk.W)
        self.topo_var = tk.StringVar(value="temp/ramanujan_80_6_adjlist.txt")
        tk.Entry(frm, textvariable=self.topo_var, width=48).grid(row=1, column=1, sticky=tk.W)
        tk.Button(frm, text="Browse", command=self._pick_topo).grid(row=1, column=2, padx=6)

        # Traffic mode
        tk.Label(frm, text="Traffic mode:").grid(row=2, column=0, sticky=tk.W)
        self.traffic_mode_var = tk.IntVar(value=0)
        tk.OptionMenu(frm, self.traffic_mode_var, -1, 0).grid(row=2, column=1, sticky=tk.W)

        # Traffic file (used when traffic_mode == -1)
        tk.Label(frm, text="Traffic file (if mode -1):").grid(row=3, column=0, sticky=tk.W)
        self.traffic_file_var = tk.StringVar(value="temp/traffic.csv")
        tk.Entry(frm, textvariable=self.traffic_file_var, width=48).grid(row=3, column=1, sticky=tk.W)
        tk.Button(frm, text="Browse", command=self._pick_traffic_file).grid(row=3, column=2, padx=6)

        # Route LUT mode
        tk.Label(frm, text="Route LUT mode:").grid(row=4, column=0, sticky=tk.W)
        self.route_lut_var = tk.IntVar(value=1)
        tk.OptionMenu(frm, self.route_lut_var, 0, 1, 2).grid(row=4, column=1, sticky=tk.W)

        # seed, ppp, packets
        tk.Label(frm, text="Seed:").grid(row=5, column=0, sticky=tk.W)
        self.seed_var = tk.IntVar(value=0)
        tk.Entry(frm, textvariable=self.seed_var, width=10).grid(row=5, column=1, sticky=tk.W)

        tk.Label(frm, text="ppp (per 10000):").grid(row=5, column=1, sticky=tk.E)
        self.ppp_var = tk.IntVar(value=100)
        tk.Entry(frm, textvariable=self.ppp_var, width=8).grid(row=5, column=2, sticky=tk.W)

        tk.Label(frm, text="packets_num:").grid(row=6, column=0, sticky=tk.W)
        self.packets_var = tk.IntVar(value=100)
        tk.Entry(frm, textvariable=self.packets_var, width=10).grid(row=6, column=1, sticky=tk.W)

        # root_select
        tk.Label(frm, text="Root select (0:fix,1:rand,2:optimal):").grid(row=7, column=0, sticky=tk.W)
        self.root_select_var = tk.IntVar(value=2)
        tk.OptionMenu(frm, self.root_select_var, 0, 1, 2).grid(row=7, column=1, sticky=tk.W)

        # path diversity
        tk.Label(frm, text="Path diversity (-1: none, 0: minimal, 1: non-minimal):").grid(row=8, column=0, sticky=tk.W)
        self.pd_var = tk.IntVar(value=0)
        tk.OptionMenu(frm, self.pd_var, -1, 0, 1).grid(row=8, column=1, sticky=tk.W)

        # load balance
        tk.Label(frm, text="Load balance (0:eq,1:local,2:non-local):").grid(row=9, column=0, sticky=tk.W)
        self.lb_var = tk.IntVar(value=0)
        tk.OptionMenu(frm, self.lb_var, 0, 1, 2).grid(row=9, column=1, sticky=tk.W)

        # path-log file
        tk.Label(frm, text="Path-log output file:").grid(row=10, column=0, sticky=tk.W)
        self.pathlog_var = tk.StringVar(value="results/gui_pathlog.txt")
        tk.Entry(frm, textvariable=self.pathlog_var, width=48).grid(row=10, column=1, sticky=tk.W)
        tk.Button(frm, text="Choose folder", command=self._choose_results_folder).grid(row=10, column=2, padx=6)

        # traffic_num (used for reading traffic file matrix size)
        tk.Label(frm, text="traffic_num (matrix size, when file used):").grid(row=11, column=0, sticky=tk.W)
        self.traffic_num_var = tk.IntVar(value=80)
        tk.Entry(frm, textvariable=self.traffic_num_var, width=10).grid(row=11, column=1, sticky=tk.W)

        # Buttons
        btnfrm = tk.Frame(self)
        btnfrm.pack(fill=tk.X, padx=8, pady=4)
        tk.Button(btnfrm, text="Run", command=self._on_run, bg="#4CAF50", fg="white").pack(side=tk.LEFT, padx=6)
        tk.Button(btnfrm, text="Stop", command=self._on_stop, bg="#F44336", fg="white").pack(side=tk.LEFT, padx=6)
        tk.Button(btnfrm, text="Open results dir", command=self._open_results_dir).pack(side=tk.LEFT, padx=6)

        # Output console
        self.console = scrolledtext.ScrolledText(self, state=tk.NORMAL, height=25)
        self.console.pack(fill=tk.BOTH, expand=True, padx=8, pady=6)

    def _find_default_exe(self):
        for p in DEFAULT_EXES:
            if os.path.exists(p) and os.access(p, os.X_OK):
                return p
        return DEFAULT_EXES[0]

    def _pick_executable(self):
        p = filedialog.askopenfilename(title="Select simulator executable", initialdir='.', filetypes=[("All files","*.*")])
        if p:
            self.exe_var.set(p)

    def _pick_topo(self):
        p = filedialog.askopenfilename(title="Select topology file", initialdir='.', filetypes=[("All files","*.*")])
        if p:
            self.topo_var.set(p)

    def _pick_traffic_file(self):
        p = filedialog.askopenfilename(title="Select traffic file", initialdir='.', filetypes=[("CSV","*.csv"), ("All","*.*")])
        if p:
            self.traffic_file_var.set(p)

    def _choose_results_folder(self):
        d = filedialog.askdirectory(title="Choose results folder", initialdir='results')
        if d:
            fname = os.path.join(d, "gui_pathlog.txt")
            self.pathlog_var.set(fname)

    def _open_results_dir(self):
        path = os.path.dirname(self.pathlog_var.get())
        if not path:
            path = '.'
        try:
            if os.name == 'nt':
                os.startfile(path)
            elif os.uname().sysname == 'Darwin':
                subprocess.Popen(['open', path])
            else:
                subprocess.Popen(['xdg-open', path])
        except Exception as e:
            messagebox.showinfo("Open dir", f"Could not open dir: {e}")

    def _append_console(self, text):
        self.console.insert(tk.END, text)
        self.console.see(tk.END)

    def _on_run(self):
        if self.proc is not None:
            messagebox.showwarning("Running", "A run is already in progress")
            return
        exe = self.exe_var.get().strip()
        if not exe:
            messagebox.showerror("Error", "Simulator executable not specified")
            return
        if not os.path.exists(exe):
            messagebox.showerror("Error", f"Executable not found: {exe}")
            return
        topo = self.topo_var.get().strip()
        if not topo or not os.path.exists(topo):
            messagebox.showerror("Error", "Topology file not found")
            return

        traffic_mode = int(self.traffic_mode_var.get())
        route_lut = int(self.route_lut_var.get())
        seed = int(self.seed_var.get())
        ppp = int(self.ppp_var.get())
        packets_num = int(self.packets_var.get())
        root_select = int(self.root_select_var.get())
        pd = int(self.pd_var.get())
        lb = int(self.lb_var.get())
        pathlog = self.pathlog_var.get().strip() or 'results/gui_pathlog.txt'
        traffic_file = self.traffic_file_var.get().strip() or ''
        traffic_num = int(self.traffic_num_var.get())

        # Ensure results dir exists
        outdir = os.path.dirname(pathlog)
        if outdir and not os.path.exists(outdir):
            os.makedirs(outdir, exist_ok=True)

        cmd = [exe, topo, str(traffic_mode), str(route_lut), str(seed), str(ppp), str(packets_num), str(root_select), str(pd), str(lb), pathlog, traffic_file if traffic_mode == -1 else traffic_file, str(traffic_num)]
        cmd = [c for c in cmd]
        cmd_display = ' '.join(shlex.quote(c) for c in cmd)
        self.console.delete('1.0', tk.END)
        self._append_console(f"Running: {cmd_display}\n\n")

        # Launch in thread and capture output
        def target():
            try:
                self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            except Exception as e:
                self._append_console(f"Failed to start process: {e}\n")
                self.proc = None
                return
            for line in self.proc.stdout:
                self._append_console(line)
            self.proc.wait()
            self._append_console(f"\nProcess exited with code {self.proc.returncode}\n")
            self.proc = None

        t = threading.Thread(target=target, daemon=True)
        t.start()

    def _on_stop(self):
        if self.proc is None:
            messagebox.showinfo("Stop", "No process is running")
            return
        try:
            self.proc.terminate()
            self._append_console("\nSent terminate signal to process\n")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to terminate process: {e}")

if __name__ == '__main__':
    app = SimulatorGUI()
    app.mainloop()
