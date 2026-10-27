# FIXLOG — QA fix pass on paper.md + abstract.md (2026-10-24)

Mirrors the 20 QA findings. Every numeric edit is justified by raw data, the paper's own
formulas, or the simulator source (`simulators/kangsim_final.c`, `simulators/eckang.c`).
Files touched: `paper.md`, `abstract.md`. Not touched: `CITATION.cff` (per instruction 8).

1. **§5.3 summary vs its own table.** Summary's "at r = 47 for h ≥ 2048" → "at r = 47 for
   h ∈ {2048, 8192}" (table: h = 2048 +0.4/+0.4%, h = 8192 +1.7/+1.6%, h = 32768 −21.5% — so
   32768 must be excluded; h = 8192 is within ≤ 2.5%). Contribution 3 aligned: "within ~2% at
   interval 2^47 for herds of 2048–8192 kangaroos".
2. **DPK range stated three ways.** Aligned to the correct scoping: paper.md abstract now
   "residuals ≤ 2.5% over DPK ∈ [0.025, 74] at the calibration herd and range"; §1.2
   contribution 3 now "DPK ∈ [0.025, 6.5] at interval 2^31 and DPK = 74 at the Kang-1
   configuration (2^40)" (2^31 dp-sweep tops out at dp 4 → DPK 6.505; DPK 74 occurs only at
   2^40); §5.3 body keeps [0.002, 74] for the *whole sweep* but now states explicitly that the
   ≤ 2.5% region is narrower and scoped in the summary. abstract.md already had the correct
   "[0.025, 74] at the calibration herd and range" — unchanged.
3. **§3.3 self-refuting sentence.** Raw check: 1/√(4·0.328125) = 0.873 ≡ √(H_unif/H) — the
   "naive birthday" computation as written IS the Herfindahl law, so it cannot be contrasted
   with it. Rewrote the honesty paragraph: the naive stationary-visit birthday computation gives
   the same √(H_unif/H) scaling (0.873 vs 0.87 measured) and therefore is not an independent
   check; the law remains a phenomenological fit. The false "≈ 0.57" is gone; no number invented.
4. **§6.1 (iv) wrong baseline.** "1.2641 vs 1.1537" → "1.2641 vs 1.0093" (SOTA+ v2 + mirror is
   priced against SOTA+ v2 = 1.0093, per the §6.1 table itself).
5. **§3.4 predicted gap at 2^31/dp12.** Recomputed from the paper's own RC formula with the
   §3.3 effective-DPK convention: uniform DPK = 0.0254 → overhead 4.80; effective DPK =
   0.0254·(32/18.3) = 0.0444 → overhead 3.63; gap ≈ 1.17. "predicted 0.14" → "predicted ≈ 1.17",
   with the derivation named inline. §7.3.4 updated to cite it ("≈ 1.17 predicted at 2^31/dp12");
   its existing "up to ≈ 0.11 at 2^31/dp5" (measured) was already consistent and is unchanged.
6. **§5.7 1v1 jm=4 mean/median.** The raw file (`B_1v1_jm4_clean.out`) is NOT present in this
   staging copy (searched repo-paper and all of puzzle-hq; only `out/rep2026/` REP_* files
   exist), so the contradiction could not be adjudicated from data. Mean 11.1402 ± 0.1207
   retained (it is the table-consistent K and its tight CI is incompatible with a median of
   19.38 under the paper's own right-skew statements); the unverifiable "median 19.38" removed
   and the parenthetical replaced by the qualitative heavy-tail statement that reading (ii)
   already supports. **Action item for the author: re-check the median in the archived file
   before final submission.**
7. **Spacing/band ratio.** From §5.5's own formula, spacing = 1.0625·W·3/(2h) = 3.1875·2^37 =
   2^38.67 at r = 47, h = 512 (cross-checked against the §5.5 table's r = 31 spacings, which
   match the same formula). Both occurrences of "2^38.3" → "≈ 2^38.7"; the "~90×" and "≈ 92"
   ratios were already correct.
8. **CITATION.cff placeholders.** Not touched, per instruction (filled at repo-push time).
9. **abstract.md trim.** The ~250-word ePrint submission trim added as a new section at the
   bottom of abstract.md (full abstract kept above it); banner updated to point to it. The trim
   carries the corrected DPK scoping (item 2), the hedged "first" (item 15), the 17.8%/10.0%
   phrasing (item 12), and the 0.01–0.05 op range (item 13).
10. **Status banner.** paper.md line 3: "preprint draft for independent review. Not submitted
    anywhere." → "preprint, prepared for submission to IACR ePrint." Rest of the header sentence
    intact.
11. **"70 configurations" substantiated.** Counted the Appendix A Z/p run matrix: n = 41 → 16
    (6 main + 2 paid + 4 mirror combos + A_mirror + 3×1v1); n = 32 → 28 (7 main + 2 mirror
    add-ons + 6 dp + 4 herd + 2 table + 2 js + 5×1v1); n = 48 → 26 (5 main + 6 dp + 4 esc +
    sotap_dp14_esc + dp14_js4_esc + 2 js + 2 herd + 3 herd×dp esc + M9 + mirror). 16 + 28 + 26
    = 70 — the abstracts' headline number is correct. One-line total added to §4.3.
12. **Concentration share.** Verified from §3.4's table: independent-tag gain = (1.4012 − 1.2606)
    /1.4012 = 10.0%; full shared-tag gain = (1.4012 − 1.1522)/1.4012 = 17.8% (now stated once in
    §3.4 (ii) and made self-explanatory in the abstract: "10.0% of the 17.8% total SOTA+ gain").
    At ideal: independent-tag ideal-equivalent 1.012 vs 1.15 → 12.0% (Law 3.2's full-synergy
    prediction 1.0038 → 12.7% is the *shared-tag* ideal). The wrong "≈ 14% of K at ideal"
    replaced by "12% of K at ideal — independent-tag ideal-equivalent 1.012 vs 1.15".
13. **Second-point marginal cost.** §7.2's derivation (one λ_J-multiplication + one squaring vs
    an inversion-dominated step) supports "≈ 0.01–0.05 op"; contribution 7's "0.01–0.03"
    aligned to 0.01–0.05.
14. **Sign-flip rate.** §3.2's "0.50–0.60 in our EC and Z/p diagnostics" contradicted §4.2.1's
    0.597 (EC) vs 0.498 (model). Rewritten: 0.597 attributed to EC; the model's 0.498 stated
    honestly (fair coin) with the observation that the model nevertheless reproduces the
    confinement.
15. **Hedged "first".** paper.md abstract: "We report what is, to our knowledge, the first
    independent, large-sample validation…" — matches §2.2/§3.2's existing hedging; the ePrint
    trim carries the same hedge.
16. **§9 overreach.** "every number … is a line of one of the cited output files" → "is
    traceable to a line of one of the cited output files or a stated derivation from them".
17. **§4.2.1/4.2.2/4.2.3 headings.** The three numbered list items promoted to `###` headings;
    all existing §4.2.x cross-references (lines ~307, ~453, ~518, ~698, ~1061) now resolve.
18. **Reference [2] cited.** Added at the kangaroo-method definition in §2.1:
    "[Pollard 1978; Pollard 2000]" — the 2000 manuscript is the kangaroo-specific writeup;
    no renumbering needed.
19. **§3.3 table empty cell.** "SOTA+ v2 + mirror" row's switch-condition cell filled with
    "always min-tag (as v2)".
20. **Minors.**
    (a) §5.7 jump-scale parenthetical: the 4√W–32√W figures are *minjump* bounds; now
    "minjump = 2^(⌊r/2⌋+js) … (mean jump 1.5·minjump ≈ 4.2–34√W at r = 31, per the §2.3 floor
    convention)".
    (b) Band definition: explicit sentence added after the §3.2 table — "the 'band' quoted
    throughout the paper is this mean-saturation value" (max runs 1.5–1.9× higher).
    (c) λ double-use: the addition-slope instances renamed λ_J (§4.2.2 "λ_J² = x′ + x + x_J",
    §4.4.1 "x′ = λ_J² − x − x_J", §7.2 "one λ_J-multiplication"); GLV λ (§6.2, abstract)
    untouched.
    (d) Abstract "each ± 0.018" → "each ± 0.017–0.018" (true CIs 0.0171/0.0180/0.0170).
    (e) minjump at odd r: **adjudicated from source** — `kangsim_final.c:335` and `eckang.c:66`
    both compute `1 << (r/2 + 3)` with C integer division, so minjump = 2^18 (not 2^18.5) at
    r = 31 and §4.2.2 was correct. §2.3 now states the floor convention explicitly (12√W at
    even r; 8.5√W mean at r = 31), §3.2's r = 31 parenthetical corrected from
    "[8,16)·√W, mean 12√W" to "[5.7, 11.3)·√W, mean 8.5√W", and the adjacent diffusive-growth
    sentence recomputed (8.5·10^3√W, "more than an order of magnitude larger than observed" —
    the old "12√W·10^3, four orders of magnitude" was doubly wrong: wrong mean and wrong
    order-of-magnitude arithmetic, 8.5e3/406 ≈ 21×).
    (f) §3.2 "insensitive to the step count beyond 10^4" → "only weakly dependent … (the largest
    drift in the sweep is the 2^8 row's 124→179√W from 10^4 to 10^6 steps; most rows are flat or
    declining)" — consistent with the table the sentence sits under.

**Not resolved here (1 open item):** the true median of the 1v1 jm = 4 run (item 6) — the raw
file is not in this staging copy; the impossible mean/median pairing was removed and the mean
retained. Verify against the archived `B_1v1_jm4_clean.out` before submission.

---

# Third referee round — fixes applied to paper.md (2026-10-26)

Second independent review of the October draft; all fix-first items applied. Every numeric
claim below is backed by a raw file in `out/` or by the instrument source in `simulators/`.

1. **Repository completeness (was the reviewer's item 1).** `bandm2.c` (band/period/tail
   instrument with exact cycle detection, plus the escape-rule `-esc -win` mode) and
   `kangsim_diag.c` (diagnostic simulator: `-cyclog2` online Brent long-cycle oracle with
   life-end classification, steps-in-cycle share and occupancy-by-life-step buckets; solve-
   channel counters classifying the *solving* collision as tame-wild / wild-wild / other)
   are now in `simulators/`, and the raw outputs (`out/`, 87 files incl. `cyclog2_*`,
   `bandm2_esc_r47.txt`) are included. README build/run section updated. The stale
   first-draft `paper.md` in this repo is replaced by the current eprint text.
2. **M-direction fix (§3.2/abstract).** The walk's confinement deepens as M *grows*, not
   shrinks; all directional statements corrected and the L1S2 bounce cap added: at M = 2^14
   the λ = 2 bounce has probability ≈ 1/(2M) per step, mean time to first bounce 2M = 32 768
   steps — which matches the measured mean kangaroo life (≈ 3.4·10^4 steps at h = 512/dp 14)
   and explains why short-cycle handling dominates the restart statistics.
3. **§5.4 long-cycle table replaced by direct measurement** (`out/cyclog2_simple_dp14_h512.out`,
   `-cyclog2`, 60 trials): 4.0% of kangaroo lives (1 800/45 239) enter an undetected long
   cycle; ≈ 3.1% of all steps are spent inside one; occupancy by life-step 1.4% @16k, 3.7%
   @26k, 5.2% @32k, 24% @64k. Nearly all such lives are terminated by the herd's *solve*
   (the `exit-while-in-cycle` category), not by the handler — at dp 14 the herd solves around
   its lost kangaroos. The earlier "≈ 6.6% of raw-walk lives" figure (reviewer's own raw-walk
   instrument) is consistent in order of magnitude; the herd figure is the paper's own.
   The dp ≥ 20 stall paragraphs now use the measured per-life entry chances (4.0% at dp 14,
   6.2% at dp 20, 6.9% at dp 24; per-step entry rate ≈ 2·10^-6 in all three) and the measured
   occupancy trajectories (dp 20: 24.4% of all steps in-cycle, caged fraction 75% at life-step
   128 000, ≈ 100% by 256 000; dp 24: 87.9% of all steps in-cycle, occupancy 1.000 from
   512 000 on) — the herd stall is directly observed, not modeled.
4. **Churn explanation rejected (§5.4).** The escape row's +10% residual cannot be churn
   (0.002 restarts/kangaroo); total churn is similar at h = 512 and h = 2048 while only
   h = 512 degrades — the residual is §5.5 herd geometry. Text updated.
5. **§5.5 reframed** around herd geometry (spacing/band ≈ 18 at h = 512) as the dominant
   dp ≤ 14 fragility; the earlier "cycling plus timeout" explanation for dp ≤ 14 is withdrawn
   (cycling is a few-percent contributor there). The dp 24 failure mechanism is stated as
   long-cycle occupancy (measurable), not "geometric luck".
6. **√(π/2) corrected (§3.2).** It is the expected first-repeat time *within one walk* (the
   Pollard ρ constant), not the meeting time of two independent walks (two equal pools give
   √π). §6.3 reframed around the wild–wild one-pool share: the solve-channel counters
   (`kangsim_diag.c`) measure wild–wild solves at 499/1 500 (33.3%) at n = 32 and 94/300
   (31.3%) at n = 41 (`out/cyclog2_ww_n32_t1500.out`, `out/cyclog2_ww_n41_t300.out`) —
   reproducing the reviewer's numbers exactly (deterministic default seed).
7. **Reconciliations.** The M = 2^9 explanations, the 1v1 conclusion (best 1v1 draws *beat*
   the classic DP herd like-for-like; the "median 19.38 > mean 11.14" is op-cap censoring —
   verified against `out/B_1v1_jm4_clean.out`: K = 11.1402, median = 19.3846), the
   Galbraith–Ruprai attribution (the ≈ 1.36√N constant is the negation-class/fast-inversion
   case of [G-R 2010], ePrint 2010/615; the citation is now explicit), and the floor/
   attainment wording (§7) are all updated per review.
8. **§7.3 rule of thumb** replaced with the table-driven selection rule; §7.4 updated.
9. **Escape-rule raw-walk check** (`out/bandm2_esc_r47.txt`): 16/200 escape-rule walks
   cycle within the 48 000-step window (92.0% do not) — same order as the reviewer's
   independent 182/200-not-cycled measurement.

Item 6 of the 2026-10-24 FIXLOG (jm = 4 median) is hereby closed: the archived file is in
`out/` and the paper's censoring explanation is verified against it.

---

# A/B/C/EC re-run matrix — provenance triage (2026-10-26)

The original `/tmp/deepdive-sim` batches were lost; a deterministic re-run matrix (74 rows,
default seed 0xABCDEF0123456789, canary A_sota re-verified byte-exact) regenerated `out/`.
Result: **58 rows byte-exact**, 4 EC rows exact-or-within-1.8% (the runner's parser missed
eckang's `K=` format — fixed), and the following triage:

1. **`A_sotap_mirror_paid` — manifest error, paper number stands.** The archived 1.2641 was
   produced at `-w 150` (0.15 op/step), not `-w 300`: rerun at `-w 150` reproduces
   `K_weighted(alt=150/1000)=1.2641` byte-exactly (K_main 0.9724 exact at `-w 300`).
   Paper §6.1 table relabeled: the two priced rows use 0.3 and 0.15 op/step respectively.
2. **`A_sota_mirror_paid2` — Kw formula drift, paper updated.** K_main 1.1262 is byte-exact;
   the archived Kw 1.4586 is 0.4% below the current instrument's 1.4641 (same stream — the
   alt-cost accounting changed post-archive). Paper now cites 1.4641.
3. **Seven genuinely stale rows — not reproducible under any candidate provenance** (default
   seed, explicit seeds 1–6 for the C_mirror row, and the `kangsim6` control binary): these
   predate a mid-session instrument fix. Regenerated from the verified current instrument and
   the paper updated:
   - C_mirror 2.2834 → **2.1591 ± 0.1241** (§5.5 table, §5.4 +42.9% → +35.2% vs formula)
   - C_sota_mirror 1.7977 → **1.6643 ± 0.1209** (§6.1: −2.1% → −5.4% ± 9%, still n.s.)
   - C_sota_dp16_esc 2.2652 → **2.3627 ± 0.1490** (§5.4 gap +24.6% → +19.4%)
   - C_sota_dp18 4.9955 → **4.8761 ± 0.2793** (§5.4 gap +23.4% → +22.0%)
     (cross-check: the `kangsim6` control binary independently reproduces the C_mirror
     2.1591 and dp 18 4.8761 values byte-exactly, ruling out the variant branch as the
     source of the discrepancy)
   - C_sota_dp20 10.657 → **10.4091 ± 1.6176** (§5.4)
   - C_sota_dp14_js4_esc 1.7298 → **1.7912 ± 0.1073** (§5.7: escape-rule js effect flips sign,
     ±2% — "vanishes once loops are handled" updated to "largely vanishes")
   - C_sota_k8192_dp12_esc 2.3105 → **2.2278 ± 0.1342** (§5.5 table +1.6% → −2.1%)
   - C_sota_k32k_dp16_esc 8.4238 → **8.3263 ± 0.4176** (§5.5 table −21.5% → −22.5%; §5.4 text)
   Two rows checked and cleared as NOT stale: C_sota_dp24 re-ran byte-exact (19.4077, 12/60
   solved — the paper's "48/60 censored" row stands), and the EC family reproduces exactly or
   within 1.8%. Final tally over all 74 rows: 58 byte-exact + 1 within 1.2% (k32k_dp16_esc,
   treated as stale above), 5 EC false-FAILs from the parser artifact (all verified exact or
   within 1.8%), 1 provenance-explained (A_sotap_mirror_paid, `-w 150` not 300; the second
   mirror-priced row's Kw drift stayed within tolerance), 8 stale rows regenerated,
   2 RECORD rows whose K_main matches the headline table byte-exactly (SOTA+ v0/v2 paid).
4. **§5.4 escape rows now directly measured** (`out/cyclog2_esc_dp14_h512.out`,
   `out/cyclog2_esc_dp14_h2048.out`): escape h = 512 — 4.5% of kangaroo lives end inside an
   undetected long cycle (4 563/102 624), 2.3% of all steps, occupancy 2.1% at life-step
   26 000; escape h = 2048 — 0.8%/0.4%. The reviewer's raw-walk figures (8.5% by 26k, ≈4% of
   steps) do not transfer to the herd; both rows are now the paper's own herd measurements.
   The dp 20/dp 24 simple rows were also measured directly (24.4% / 87.9% of all steps
   in-cycle; herd stall observed, not modeled).

Matrix closed: every row is now either byte-exact against the archived value or traced to a
specific provenance difference (parser artifact, pricing flag, alt-accounting change, or a
pre-fix instrument); the paper cites only reproducible numbers.

---

# Fourth referee round — fixes applied to paper.md (2026-10-26 late)

1. **Rerun disclosure.** Appendix A now carries a "Measurement changelog (this revision)"
   section: the /tmp/deepdive-sim loss, the 74-row deterministic re-run matrix, 58 byte-exact
   rows, the 8(+1) superseded rows with old→new values, the two provenance-explained rows,
   the EC parser false-FAILs, and the kangsim6/seed-scan failed hypotheses. §9's "every
   number is a line of one of the cited output files" softened to trace-to-file-or-changelog.
2. **Table consistency.** §5.3's dp16-esc (2.2652→2.3627, −0.4%→+3.9%), dp18-esc
   (4.0484→3.9981, +14.6%→+13.2%) and dp20 (10.657→10.4091, +79%→+75%) rows now agree with
   §5.4; §6.1's mirror n=48 gain sign fixed (−5.4%→+5.4% under the table's gain convention).
3. **§6.1 pricing.** The 0.15 op/step label stands (the referee's 0.3 inference assumed
   K_weighted = K_main×(1+w); the instrument's v2+mirror identity is ×(1+2w), verified from
   source: walt accrues per alt-point advance, and the -w 1000 paid rows print ×2.0). The
   text now discloses the identity and fixes the baseline comparison (1.2641 vs its SOTA+ v2
   baseline 1.0093, not SOTA 1.1537).
4. **§5.4 counting basis.** "Life"/denominator defined once (kangaroo-slot vs handler-ended
   life vs caged-at-solve census); the rows restated on the slot basis (5.9%/4.5%/0.8%/
   35.8%/89.5% entry shares; caged-at-solve 5.9%/4.5%/—/35.6%/89.0%); the regime-independent
   per-uncaged-step entry rate ≈1.3–2.2·10⁻⁶ replaces the "4–7% per fresh life" reading;
   the earlier 4.0%/6.2%/6.9% withdrawn as a mixed denominator.
5. **Small points.** §2.1: G-R "fast inversion" = cheap point negation (not batched field
   inversion), aligned with §6.2. §6.2: Churchwell attainability vs floor distinguished.
   §7.4: "bound from below" → "measure directly"; lettering checked (a/b/c/d correct here —
   the referee's duplicate-(b) sighting did not reproduce; flagged for re-check in PDF).
6. **Repository.** The remote github.com/decruptor/kangaroo-constants still served the FIRST
   draft to the referee (this repo kit's updates are local; the push awaits explicit
   approval). Local kit now holds: current paper.md, README, FIXLOG, bandm2.c,
   kangsim_diag.c, out/ (87+ files), and the changelog above.

---

# Fifth referee round — re-run triage corrected, fixes applied (2026-10-27)

## The re-run triage above (2026-10-26) was wrong; corrected record

The "provenance triage" section above diagnosed nine rows as "pre-fix instrument artifacts"
and replaced their first-draft values. That diagnosis is withdrawn. Its re-run manifest had
been reconstructed from the paper's text instead of taken from the run record. Recovering
the actual first-draft command lines from the session record of the 2026-10-22/23 runs shows
16 of the 74 manifest commands were wrong:

| row | original command (as run) | mis-reconstructed |
|---|---|---|
| C_sota_dp12 / dp16 / dp18 | `-m 3 -n 48 -t 150 -k 512 -d {12,16,18}` | `-t 300` |
| C_sota_dp20 | `-t 80` | `-t 60` |
| C_mirror | `-m 6 -n 48 -t 150` | `-t 300` |
| C_sota_mirror | `-m 3 -mirror -n 48 -t 150 -d 14` (simple handling) | added `-esc` |
| C_sota_dp16_esc / dp18_esc | `-t 150` / `-t 100` | `-t 200` |
| C_sota_k8192_dp12_esc, C_sota_k32k_dp16_esc, C_sota_dp14_js4_esc | `-t 300` | `-t 200` |
| A_mirror | `-m 6 -n 41 -t 3000` | `-t 4000` |
| A_sota_mirror_paid2 | `-m 3 -mirror -w 300 -n 41 -t 3000` | `-t 4000` |
| B_sota_k8192 | `-m 3 -n 32 -t 20000 -k 8192` | `-t 12000` |
| A_sotap_mirror_paid | `-w 150` | `-w 300` |
| EC R = 32, 30 trials | `eckang -m 4 -v 2 -t 30 -R 32` (SOTA+ v2) | `-m 3` (SOTA) |

Re-run with the original commands, all 16 reproduce the first draft's printed values digit
for digit. No instrument change was involved; `kangsim_final.c` is the production source
(the first-draft binaries `kangsim4`/`kangsim5` were snapshots of it, and rows from both
reproduce). The n = 48 clustering only reflects which batches were mis-reconstructed. The
earlier seed-scan and `kangsim6` "controls" were run on the wrong commands and prove nothing.

Restored in paper.md: C_mirror 2.2834 ± 0.1882 (+42.9%, restarts/kang 1.56); dp 16 escape
2.2652 ± 0.1582 (−0.4%, so the §5.5 sentence is correct again); dp 18 simple 4.9955 and
escape 4.0484 ± 0.3290 (+14.6%); dp 20 10.657 ± 1.5753 (+79%); simple-vs-escape gaps
+24.6% (dp 16) and +23.4% (dp 18); js 4 escape 1.7298 (−1.7%; "flips sign" removed);
h = 8192 dp 12 escape 2.3105 ± 0.1069 (+1.6%); h = 32768 dp 16 escape 8.4238 ± 0.3572
(−21.5%); SOTA + mirror n = 48 1.7977 ± 0.1402. The last is now compared with its
like-for-like simple-handling baseline (C_sota 1.9502 ± 0.1025): +7.8% ± 9%, not significant.
Kept: §6.1 n = 41 priced 1.4641 (same 4 000-trial run as the row's K_main 1.1262); the
first draft's 1.4586 came from the 3 000-trial run, K_main 1.1220, and also reproduces.

`out/` now holds the original-command outputs; `out/MANIFEST.txt` lists the exact command for
every file.

## jm = 4 1v1 median (§5.7)

The referee's rerun of the first 2 000 trials gives median 6.43; the full 20 000-trial run
reproduces the first draft's 19.3846 exactly. Both are right. With one jump table per run,
all trials walk the same coalescence tree; at jm = 4 the tree has one dominant merge point
(the group element 85 542 313 345, 39.8W out), and the 10 006 of 20 000 trials whose tame and
wild trails start in different main branches all end there (K = 19.38–19.87). The rest end far
earlier. The median sits on the boundary of the two modes, so it is unstable. The previous
revision's censoring explanation is withdrawn; no trial reaches the op cap.

## Other fixes

- Changelog wording: "digit for digit in the printed fields", not "byte-exact".
- §5.4: the long-cycle entry rate is described as an age-dependent hazard, not
  "regime-independent".
- §6.1: the priced SOTA+ v2 + mirror row is compared with SOTA+ v2 priced the same way
  (1.0093 × 1.15 ≈ 1.16).
- §7.4: the open problems render as (a)–(d); a line-initial "(c)" had been parsed as a
  one-item list and printed as "(b)".
- Canonical text: this repository's paper.md, which is the exact source of the revised PDF.
- Repository: out/ (with MANIFEST.txt), bandm2.c, kangsim_diag.c and this FIXLOG now ship.
  The earlier .gitignore excluded out/ and *.out, which is why they were never published.
