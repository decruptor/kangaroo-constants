#!/usr/bin/env python3
# Pool archived production runs (default seed) with the fresh-seed replication chunks.
# Methodology per paper S3.4 + S4.4:
#  - the 200-trial pilot is NOT pooled (S3.4 warning);
#  - production runs only; old runs reproduced byte-exact on the default seed;
#  - pooled mean = sum(n_i*m_i)/sum(n_i);
#  - headline CI in the paper's own convention: 1.96*sd_pooled/sqrt(N), sd_pooled the
#    within-run pooled sd (each run's sd is within-run: table effect is fixed within a run);
#  - cross-seed (jump-table) component quantified separately from the scatter of the
#    fresh-seed chunk means (S4.4.2), added as a conservative CI variant.
import re, sys, math, glob, os

OUT = os.path.dirname(os.path.abspath(__file__))

def parse_sim(path):
    txt = open(path).read()
    m = re.search(r"solved (\d+)/(\d+)\s+K_main=([\d.]+) \+/- ([\d.]+).*?sd=([\d.]+)", txt)
    dp = re.search(r"dp_rate=1/([\d.]+)", txt)
    gave = len(re.findall(r"GAVE UP", txt))
    hdr = re.search(r"# method=\S+ n=(\d+).*?trials=(\d+) v=(\d+).*?it=(\d+)", txt)
    return dict(solved=int(m.group(1)), trials=int(m.group(2)), K=float(m.group(3)),
                ci=float(m.group(4)), sd=float(m.group(5)), dp=dp.group(1) if dp else None,
                gave=gave, n=hdr.group(1), v=hdr.group(3), it=hdr.group(4))

def pool(runs, label):
    N = sum(r["solved"] for r in runs)
    mean = sum(r["solved"] * r["K"] for r in runs) / N
    ss = sum((r["solved"] - 1) * r["sd"] ** 2 for r in runs)
    sd_p = math.sqrt(ss / (N - len(runs)))
    ci = 1.96 * sd_p / math.sqrt(N)
    print(f"\n== {label} ==")
    for r in runs:
        print(f"   n={r['solved']:>6}  K={r['K']:.4f}  sd={r['sd']:.4f}  dp=1/{r['dp']}  ({r['src']})")
    print(f"  POOLED: n={N}  K={mean:.4f} +/- {ci:.4f} (95% CI, paper convention: 1.96*sd/sqrt(N))")
    # cross-seed component from fresh chunk means (exclude the single default-seed run)
    fresh = [r for r in runs if r.get("fresh")]
    if len(fresh) >= 3:
        ms = [r["K"] for r in fresh]
        nb = len(ms); mb = sum(ms) / nb
        var_between = sum((x - mb) ** 2 for x in ms) / (nb - 1)
        nchunk = fresh[0]["solved"]
        tau2 = var_between - sd_p ** 2 / nchunk
        if tau2 > 0:
            w2 = sum((r["solved"] / N) ** 2 for r in runs)
            se_extra = math.sqrt(tau2 * w2)
            ci_total = math.sqrt(ci ** 2 + (1.96 * se_extra) ** 2)
            print(f"  cross-seed (jump-table) component: tau={math.sqrt(tau2):.4f} "
                  f"-> adds {1.96*se_extra:.4f} to the CI; conservative CI = +/- {ci_total:.4f}")
        else:
            print(f"  cross-seed component: not detectable (scatter of {nb} fresh-seed chunk means "
                  f"consistent with within-run sd); tau^2 estimate {tau2:.2e} clipped at 0")
    return mean, ci, N, sd_p

arms = {
    "IT_v2 (SOTA+ independent tag, flagged item)": ("REP_IT_v2_s*.out", (12000, 1.2653, 0.5842, "32.0"), "IT_v2_n32 (archived, default seed)"),
    "SH_v2 (SOTA+ shared tag)": ("REP_SH_v2_s*.out", (20000, 1.1522, 0.5718, "18.3"), "B_sotap_v2 (archived, default seed)"),
    "IT_v0 (SOTA+ v0 independent tag)": ("REP_IT_v0_s*.out", (12000, 1.2626, 0.5815, "32.0"), "IT_v0_n32 (archived, default seed)"),
}

results = {}
for label, (pat, old, oldname) in arms.items():
    files = sorted(glob.glob(os.path.join(OUT, pat)))
    runs = [dict(src=oldname, solved=old[0], K=old[1], sd=old[2], dp=old[3])]
    bad = []
    for f in files:
        r = parse_sim(f); r["src"] = os.path.basename(f); r["fresh"] = True
        if r["solved"] != 4000 or r["trials"] != 4000 or r["gave"]:
            bad.append((f, r))
        runs.append(r)
    for f, r in bad:
        print(f"  WARNING: {os.path.basename(f)} solved={r['solved']}/{r['trials']} gave_up={r['gave']}", file=sys.stderr)
    results[label] = pool(runs, label)

# sanity: v2 IT vs SH gap
it = results["IT_v2 (SOTA+ independent tag, flagged item)"]
sh = results["SH_v2 (SOTA+ shared tag)"]
gap = sh[0] - it[0]
se = math.sqrt(sh[1] ** 2 + it[1] ** 2) / 1.96
print(f"\nshared-vs-independent gap (pooled): {gap:.4f} +/- {1.96*se:.4f}  (S3.4 quoted 0.113 measured, 0.105 predicted)")
