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
