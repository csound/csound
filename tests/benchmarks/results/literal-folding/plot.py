"""Draw the saved benchmark measurements. Requires matplotlib.

python3 plot.py OUTPUT_DIR MACOS_REPORT_JSON LINUX_REPORT_JSON

The HTML files contain the complete SVG figures. No timing data is regenerated.
"""
import argparse
import html
import io
import json
from pathlib import Path
import re

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

DATA = {}
PAPER, INK, MUTED, ACCENT = "#f5f5f5", "#2d3142", "#4f5d75", "#bf4520"
LABELS = ["Musical", "Dynamic 64", "Literal 64", "Literal 512"]
HOSTS = [("macos-double", "Apple M5 Max"), ("linux-double", "Ryzen 9 7950X")]
plt.rcParams.update({"figure.facecolor": PAPER, "axes.facecolor": PAPER,
    "text.color": INK, "axes.labelcolor": MUTED, "xtick.color": MUTED,
    "ytick.color": MUTED, "font.size": 11, "svg.fonttype": "none",
    "svg.hashsalt": "literal-folding", "axes.spines.top": False,
    "axes.spines.right": False, "axes.spines.left": False})


def axes_style(ax):
    ax.set_axisbelow(True)
    ax.grid(axis="y", color=INK, alpha=0.08)
    ax.spines["bottom"].set_color(MUTED)
    ax.tick_params(axis="both", length=0, pad=8)


def save(fig, output, name, title, description):
    stream = io.StringIO()
    fig.savefig(stream, format="svg", metadata={"Date": None})
    svg = stream.getvalue()
    svg = svg[svg.index("<svg"):]
    svg = re.sub(r'<svg\b', '<svg role="img" aria-labelledby="' +
                 name + '-title ' + name + '-desc"', svg, count=1)
    pos = svg.index(">") + 1
    svg = (svg[:pos] + f'<title id="{name}-title">{html.escape(title)}</title>' +
           f'<desc id="{name}-desc">{html.escape(description)}</desc>' + svg[pos:])
    # Matplotlib uses DejaVu in the plot so offline renders keep readable labels.
    page = f'''<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)}</title>
<style>
body {{ margin:0; padding:24px; background:{PAPER}; color:{INK}; font:16px sans-serif; }}
main {{ max-width:1280px; margin:auto; min-width:0; }}
h1 {{ font:32px Georgia,serif; }}
.diagram-container {{ width:100%; overflow-x:auto; }}
svg {{ display:block; width:100%; min-width:1280px; height:auto; }}
p {{ max-width:80ch; line-height:1.5; }}
@media print {{ .diagram-container {{overflow-x:visible}} svg {{min-width:0}} }}
</style></head><body><main>
<h1>{html.escape(title)}</h1><p>{html.escape(description)}</p>
<div class="diagram-container">{svg}</div>
<p>Measured on 26 October 2026. Default light palette with offline system fonts.
Raw trials and build flags are in the input JSON reports.</p>
</main></body></html>'''
    (output / (name + ".html")).write_text(page)
    plt.close(fig)


def timing(output):
    fig, axes = plt.subplots(2, 2, figsize=(1280/72, 720/72))
    fig.subplots_adjust(left=.08, right=.97, bottom=.12, top=.84, hspace=.43, wspace=.23)
    fig.suptitle("Less note initialization, with a compile-time cost", x=.08,
                 y=.97, ha="left", fontsize=24, fontfamily="serif")
    fig.text(.08, .91, "128 simultaneous notes  /  Process CPU time  /  Lower is better", color=MUTED)
    for col, (host, label) in enumerate(HOSTS):
        rows = DATA[host]["results"]
        for row, (metric, stage) in enumerate([
                ("compile_cpu_ms", "Compilation"), ("activation_cpu_ms", "Activation")]):
            ax = axes[row, col]
            stats = [x["summary"][metric] for x in rows]
            values = np.array([x["on_over_off_median"] * 100 for x in stats])
            ci = np.array([x["paired_bootstrap_95pct"] for x in stats]) * 100
            ax.bar(LABELS, values, width=.6, color=[MUTED]*3+[ACCENT], alpha=.85)
            ax.errorbar(range(4), values, yerr=[values-ci[:, 0], ci[:, 1]-values],
                        fmt="none", color=INK, capsize=4)
            ax.axhline(100, color=INK, linewidth=1, linestyle="--")
            ax.set_ylim(0, 140)
            ax.set_yticks([0, 50, 100])
            ax.set_ylabel("% of pass-off time")
            ax.set_title(label + " · " + stage, loc="left", fontsize=13, pad=18)
            for i, value in enumerate(values):
                ax.text(i, ci[i, 1]+5, f"{value:.1f}%", ha="center", fontsize=11)
            axes_style(ax)
    fig.text(.08, .035, "Mask 1 → mask 3. Median of 21 paired ratios; whiskers show 95% bootstrap intervals.", color=MUTED)
    save(fig, output, "timing", "Literal folding timing",
         "The large literal cases initialize faster. The largest Linux case compiles about 22% slower. "
         "Activation includes allocation, initialization and one audio block. The small musical case shows no clear activation gain.")


def memory(output):
    fig, axes = plt.subplots(1, 2, figsize=(1280/72, 720/72))
    fig.subplots_adjust(left=.08, right=.97, bottom=.22, top=.78, wspace=.23)
    fig.suptitle("Lower process peak memory in the large literal cases", x=.08,
                 y=.97, ha="left", fontsize=24, fontfamily="serif")
    fig.text(.08, .91, "128 simultaneous notes  /  Median peak RSS after activation  /  Fresh process per trial", color=MUTED)
    for ax, (host, label) in zip(axes, HOSTS):
        for mode, (key, color, shift) in enumerate([
                ("off_median", MUTED, -.18), ("on_median", ACCENT, .18)]):
            values = [r["summary"]["activation_peak_rss_bytes"][key]/2**20
                      for r in DATA[host]["results"]]
            bars = ax.bar(np.arange(4)+shift, values, width=.32, color=color,
                          alpha=.85, label="Pass off" if mode == 0 else "Pass on")
            ax.bar_label(bars, fmt="%.1f", padding=5, fontsize=10)
        ax.set_xticks(range(4), LABELS)
        ax.set_ylim(0, 70)
        ax.set_ylabel("Process peak RSS (MiB)")
        ax.set_title(label, loc="left", fontsize=13, pad=18)
        axes_style(ax)
    axes[0].legend(loc="upper left", frameon=False)
    fig.text(.08, .10, "Includes Python, libraries, compiler storage and allocator overhead. Cumulative high-water mark, not live allocation.", color=MUTED)
    fig.text(.08, .06, "The pass leaves variable-pool slots in place. These results do not measure per-note variable storage.", color=MUTED)
    save(fig, output, "memory", "Literal folding memory",
         "Compare pass-off and pass-on process peaks within each host. These measurements do not isolate Csound allocations "
         "and are not a memory trace over time.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("macos_report", type=Path)
    parser.add_argument("linux_report", type=Path)
    args = parser.parse_args()
    DATA = {"macos-double": json.loads(args.macos_report.read_text()),
            "linux-double": json.loads(args.linux_report.read_text())}
    output = args.output
    output.mkdir(parents=True, exist_ok=True)
    timing(output)
    memory(output)
