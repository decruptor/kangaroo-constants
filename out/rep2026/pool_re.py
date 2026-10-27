#!/usr/bin/env python3
# Random-effects pooling: within-run sd (sampling) + between-run (jump-table draw) component.
# DerSimonian-Laird method-of-moments over runs (archived default-seed run + fresh-seed chunks).
import math, glob, os, re

OUT = os.path.dirname(os.path.abspath(__file__))

def parse_sim(path):
    txt = open(path).read()
    m = re.search(r"solved (\d+)/(\d+)\s+K_main=([\d.]+) \+/- ([\d.]+).*?sd=([\d.]+)", txt)
    dp = re.search(r"dp_rate=1/([\d.]+)", txt)
    return dict(n=int(m.group(1)), m=float(m.group(3)), sd=float(m.group(5)),
                dp=dp.group(1) if dp else None, gave=len(re.findall(r"GAVE UP", txt)))

def re_pool(runs, label):
    # runs: list of (n, mean, sd)
    R = len(runs)
    # fixed-effect start
    def fe(tau2):
        ws = [1.0 / (sd**2 / n + tau2) for (n, m, sd) in runs]
        mu = sum(w * m for w, (_, m, _) in zip(ws, runs)) / sum(ws)
        return ws, mu
    ws0, mu0 = fe(0.0)
    Q0 = sum(w * (m - mu0) ** 2 for w, (_, m, _) in zip(ws0, runs))
    if Q0 > (R - 1):
        # heterogeneity present; DL can oscillate with few runs -> take the positive branch
        sw, sw2 = sum(ws0), sum(w * w for w in ws0)
        tau2 = max(0.0, (Q0 - (R - 1)) / (sw - sw2 / sw))
    else:
        tau2 = 0.0
    ws, mu = fe(tau2)
    se = math.sqrt(1.0 / sum(ws))
    ci = 1.96 * se
    N = sum(n for (n, _, _) in runs)
    sd_within = math.sqrt(sum((n - 1) * sd**2 for (n, _, sd) in runs) / (N - R))
    print(f"{label}: N={N}  runs={R}  pooled K={mu:.4f} +/- {ci:.4f} (RE 95% CI)"
          f"  [within-run-only CI would be +/- {1.96*sd_within/math.sqrt(N):.4f}]"
          f"  tau(table draw)={math.sqrt(tau2):.4f}")
    return mu, ci, N

arms = {
    "IT_v2": ((12000, 1.2653, 0.5842), "REP_IT_v2_s*.out"),
    "SH_v2": ((20000, 1.1522, 0.5718), "REP_SH_v2_s*.out"),
    "IT_v0": ((12000, 1.2626, 0.5815), "REP_IT_v0_s*.out"),
}
res = {}
for tag, (old, pat) in arms.items():
    runs = [old]
    for f in sorted(glob.glob(os.path.join(OUT, pat))):
        r = parse_sim(f)
        assert r["gave"] == 0 and r["n"] == 4000, f
        runs.append((r["n"], r["m"], r["sd"]))
    res[tag] = re_pool(runs, tag)

it, sh, v0 = res["IT_v2"], res["SH_v2"], res["IT_v0"]
gap = it[0] - sh[0]
gapci = 1.96 * math.sqrt((it[1] / 1.96) ** 2 + (sh[1] / 1.96) ** 2)
print(f"\ngap independent-vs-shared (pooled): {gap:.4f} +/- {gapci:.4f}")
print(f"IT_v2 residual vs 1.2525 prediction: {(it[0]-1.2525)/1.2525*100:+.2f}%")
print(f"SH_v2 residual vs 1.1473 prediction: {(sh[0]-1.1473)/1.1473*100:+.2f}%")
print(f"IT_v0 residual vs 1.2773 prediction (1.0286 ideal + 0.2487 overhead): {(v0[0]-1.2773)/1.2773*100:+.2f}%")
print(f"IT_v2 vs SOTA 1.4012: {100*(1-it[0]/1.4012):.1f}% below")
print(f"ideal-equiv IT_v2 = {it[0]-0.2487:.4f}   IT_v0 = {v0[0]-0.2487:.4f}")
