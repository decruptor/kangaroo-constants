# An empirical analysis of Pollard-kangaroo walk constants for interval ECDLP on secp256k1, including the unpublished SOTA family

**Status:** preprint draft for independent review. **Not submitted anywhere.**
**Author:** Decruptor (pseudonymous) — the simulation and analysis tooling was produced with the assistance of Claude Code; see §Acknowledgements.
**Date:** 2026-10-23. **Code and raw data:** `/tmp/deepdive-sim` snapshots referenced in Appendix A; the artifacts analyzed here are third-party (RetiredCoder) plus the authors' simulator.

---

## Abstract

We report the first independent, large-sample validation of the walk constants of the "SOTA" and
"SOTA+" kangaroo methods for interval ECDLP on secp256k1 — an unpublished family due to RetiredCoder
(RC) that underlies the fastest publicly known solvers for the Bitcoin-puzzle ranges. Using a
hash-oracle model of the group, we run ~5.1·10^5 full DLP solves across 70 configurations at
interval widths 2^31–2^47 and reproduce all published constants: SOTA **K = 1.15** (measured
1.1537 ± 0.0195 at the exact Kang-1 configuration), SOTA+ **1.02 / 1.05 / 0.99** (measured 1.0242,
1.0548, 1.0093, each ± 0.018), classic ≈ 2.1 and 3-way ≈ 1.6. We validate RC's distinguished-point
overhead formula `K = 1.15 + (0.07 + 0.76/√DPK)/(1 + 0.30·DPK)` over four orders of magnitude of
DPK ∈ [0.002, 74] and chart its failure regimes quantitatively.

The mechanism analysis has three parts. (i) *Walk confinement:* the SOTA walk is not a diffusion;
its displacement saturates at a band of ≈ 350–800·√W within ~10^3–10^4 steps, which explains why
reduced start ranges (tames in the bottom W/32 of the interval) still work and why loop handling,
not range size, governs large-range behavior. (ii) *Selection-rule biasing:* the SOTA+ branch tag
shares low bits with the jump index and the DP test, so the selection rule concentrates the walk on
distinguished states; we derive the resulting per-step DP rate exactly (1/18.29 predicted vs
1/18.3 measured for the GPU rule) and an empirical Herfindahl law K/K_SOTA ≈ √(H_unif/H_sel) that
reproduces all SOTA+ constants to ≤ 1%. (iii) *Decomposition:* re-keying the branch tag to an
independent hash collapses the DP rate to exactly 1/32 and removes precisely the DP-overhead share
of the gain, while the concentration share survives (9.7% vs 17.8% at 2^31/dp5; statistically
inseparable at the Kang-1 configuration, Δ = 0.003 ± 0.024). A 200-trial pilot of this control
suggested the opposite conclusion and is documented as a methodological warning.

We quantify two fragility conditions: loop handling (naive "restart on revisit" degrades 1.15 →
1.95 at 2^47, and to 4.7 with a small jump table) and herd-size scaling (the DP formula holds only
while wild starts tile the interval at least as densely as the saturation band; at 2^47 with 512
kangaroos and DP bits 24, 48 of 60 trials fail outright). Two negative results: a correlated
"mirror" second walking sample gains ≤ 3.6% ± 1.3% at zero marginal cost and is unprofitable at any
physically realizable price; and 1-D endomorphism-accelerated walks (Galbraith–Ruprai style) are
obstructed on secp256k1 because GLV λ-orbits preserve no interval of width W ≪ n — bearing on an
unpublished preprint claiming a √(π/2) ≈ 1.2533 floor for 1-D walks. Finally, an honest accounting
of SOTA+: it beats plain SOTA only when the second point costs < ≈ 0.14–0.22 operations per step —
true for unbatched CPU inversion, false on batched-inversion GPUs, which is why RC's own GPU
implementation ships plain SOTA only.

---

## 1 Introduction

### 1.1 The problem and the practice

Given a base point G of prime order n on an elliptic curve (here secp256k1) and a point Q = kG with
k known to lie in an interval of width W = 2^r, the interval (or "bounded") discrete-logarithm
problem can be solved in O(√W) group operations by Pollard's kangaroo (lambda) method
[Pollard 1978; van Oorschot–Wiener 1999]. The constant hidden in the O-notation — the ratio of the
expected number of group operations to √W, which we write K throughout — is the entire practical
difference between methods, because memory for distinguished points (DPs) scales with the same
budget. On secp256k1 this problem is not academic: the Bitcoin-puzzle addresses (public keys whose
private keys lie in [2^(m-1), 2^m)) have sustained a decade-long, money-priced solving effort, and
the fastest publicly known solver, RCKangaroo [RC, RCKangaroo], reports ~14.5 GH/s on an RTX 4090
with K = 1.15.

The theoretical lineage of K is short and mostly two decades old: the two-herd kangaroo sits at
K ≈ 2 (heuristic 2 + o(1) [GPR 2010]); four herds bring 1.715 [GPR 2010]; a Gaudry–Schost-style
variant 1.661 [GPR 2010]; equivalence classes with fast inversion bring ≈ 1.36 [G-R 2010]. No
published method goes below 1.36 for this problem, and community folklore held for years that
~2 was the practical constant on secp256k1.

In 2024–2025 the pseudonymous author RetiredCoder published (in source-code form, with minimal
write-ups, on GitHub and the Bitcointalk research thread) a family of methods he calls SOTA and
SOTA+, with claimed constants K = 1.15 and — using a "cheap second point" — K ≈ 1.02/1.05/0.99
depending on the selection rule and the assumed cost of the second point [RC, Kang-1; RC, Kang-2].
These constants sit far below the published lineage. The methods are unpublished in the
academic sense; their correctness and constants are verified only by RC's own code and by community
use. Because these constants directly determine the economics of puzzle solving — and of every
other use of interval ECDLP, friendly or otherwise — someone eventually had to measure them
properly. This paper is that measurement: ~5.1·10^5 full DLP solves, most of them spent confirming
another person's README. Someone had to, and the queue was short.

### 1.2 Contributions

1. **Independent reproduction of the unpublished constants** (§5.1, §5.2). At the exact Kang-1
   configuration (interval 2^40, 512 kangaroos, DP bits 5, jump table 2^14) we measure:

   | method | RC published | this work (4000 trials) |
   |---|---|---|
   | classic (herd) | ≈ 2.1 | 2.1113 ± 0.0348 |
   | 3-way | ≈ 1.6 | 1.6141 ± 0.0284 |
   | SOTA | 1.15 | 1.1537 ± 0.0195 |
   | SOTA+ v0 (second point free) | 1.02 | 1.0242 ± 0.0171 |
   | SOTA+ v1 (pay (1MUL+1SQR)/4) | 1.05 | 1.0548 ± 0.0180 |
   | SOTA+ v2 (pay 1MUL+1SQR; "GPU") | 0.99 | 1.0093 ± 0.0170 |

   All agree with the published values within ±0.02. At interval width 2^31 (20 000 trials per
   configuration) the same methods measure 2.3846, 1.9003, 1.4012, 1.1968, 1.2667, 1.1522 — the
   departures from the published ideal constants being mostly the DP overhead at that
   configuration (§5.3; the legacy methods retain a +5–7% geometry load there, §5.1).

2. **A stationary-distribution model of the SOTA+ selection rule** (§3.3) that (a) predicts the
   per-step DP rate exactly — 1/32 (SOTA), 1/21.33 (v0), 1/25.6 (v1), 1/18.29 (v2), measured
   1/32.0, 1/21.3, 1/25.6, 1/18.3 — and (b) reproduces all SOTA/SOTA+ constants to ≤ 1% via a
   one-parameter Herfindahl scaling law, once the DP overhead is accounted with RC's own formula.

3. **Validation and delimitation of RC's DP-overhead formula** (§5.3). The formula reproduces our
   measurements within ~2.5% over DPK = ideal DPs per kangaroo ∈ [0.025, 74] at interval 2^31 and
   within ~2% at interval 2^47 for sufficiently large herds; we chart where it fails (+8–13% at
   2^47 with 512 kangaroos and DP bits ≤ 14; growing failures at DP bits ≥ 18; −22% at 32 768
   kangaroos) and identify the geometric quantity that governs the failures (wild-start spacing vs
   walk saturation band, §5.5).

4. **Mechanism analysis** (§3.2, §3.3, §3.4): walk confinement (saturation band measured at
   ≈ 350–800·√W for jump tables 2^12–2^16, saturating within ~10^3–10^4 steps), the exact
   selection-induced DP-rate identities, the Herfindahl scaling, and a control experiment that
   decomposes the SOTA+ gain into a concentration component (survives an independent-hash tag) and
   a DP-rate component (vanishes: 1/18.3 → 1/32.0, exactly).

5. **Fragility quantification** (§5.4, §5.5): loop handling (naive restarts: K = 1.95 at 2^47;
   chain-preserving escape: 1.76; small jump table M = 2^9: 4.70), the DP-bit cliff (DP bits 24 at
   2^47 with 512 kangaroos: 48/60 trials fail), and the herd-size law.

6. **Negative results** (§6): the correlated-mirror second sample (≤ 3.6% ± 1.3% gain, unprofitable
   at any price); the λ-containment obstruction for 1-D endomorphism walks; and an assessment of
   the claimed √(π/2) ≈ 1.2533 "floor" (§6.3), including why SOTA+ sitting arithmetically below it
   is not a contradiction.

7. **Honest accounting of SOTA+** (§7.2): the second point must cost < 0.14–0.22 op/step to beat
   plain SOTA. This holds when inversion dominates the step cost (unbatched CPU arithmetic:
   extra ≈ 0.01–0.03 op) and fails when inversion is batched away (GPU: extra ≈ 0.3–0.7 op). RC's
   own GPU implementation ships plain SOTA, not SOTA+ — we verify this in the kernel source — so
   the GPU-relevant constant is 1.15, not 0.99.

### 1.3 What this paper is not

It is not a proposal of a new method; every method analyzed is RC's, and our only algorithmic
additions are controls (independent-tag oracle, mirror variants) designed to falsify explanations.
It is not a proof; all statements about constants below are empirical at the stated sample sizes,
with binomial/normal confidence intervals on the mean of the per-trial cost distribution, which is
heavy-tailed (we report medians as well). It is not adversarially robust: our hash-oracle model is
faithful to secp256k1 in the measured respects listed in §4.4 and unfaithful in the respects listed
there too. And it is not the last word: RC reads the same thread we do, and the constants in this
family have a demonstrated habit of improving whenever someone declares a floor.

---

## 2 Background

### 2.1 Interval ECDLP and kangaroo methods

Let G generate a cyclic group of prime order n (for secp256k1, n ≈ 2^256), and let Q = kG with
k ∈ [a, a+W). Pollard's kangaroo method [Pollard 1978] runs two "kangaroos": a *tame* starting at a
known multiple tG and a *wild* starting at Q + offset. Both perform pseudo-random walks with jumps
drawn from a fixed table {J_0, …, J_{M-1}} of known multiples of G; the jump index is a
deterministic function of the current point (in practice, low bits of the x-coordinate modulo M).
Whenever a walk lands on a *distinguished point* (DP — a point whose x-coordinate satisfies an
easily testable property, e.g. low d bits zero), its displacement is stored in a table. A collision
between a stored tame and a stored wild displacement yields k by linear algebra. Each jump is one
group operation; the total expected cost is K·√W with K determined by the walk rule, the herd
geometry and the DP overhead.

van Oorschot and Wiener's parallelization [vOW 1999] replaces the two kangaroos by two herds of h
kangaroos each; the expected total work remains K·√W group operations with K ≈ 2 for the classic
directed walk, while the DP storage requirement scales as √W/2^d for DP bits d.

Galbraith, Pollard and Ruprai [GPR 2010] introduced multi-herd variants: four herds (tame, wild,
and their negations, with a halved wild range) give K = 1.715 + o(1); a Gaudry–Schost-flavoured
variant gives 1.661 + o(1). Galbraith and Ruprai [G-R 2010] applied *equivalence classes* — orbits
under an efficiently computable endomorphism — to the interval DLP and obtained ≈ 1.36√N for groups
with fast inversion, while noting explicitly that it "seems impossible" to combine equivalence
classes with the standard kangaroo method, and that in practice their algorithm runs slower than
its constant because walks leave the search space and fruitless cycles must be handled. To our
knowledge no published method for a *1-dimensional* walk has a constant below ≈ 1.36, and no
published analysis supports a constant below ≈ 1.25 for any variant; a community-circulated
unpublished preprint claims a floor/attainment of √(π/2) ≈ 1.2533 for 1-D walks (§6.3).

### 2.2 The SOTA family (RetiredCoder, unpublished)

RC's three-part release defines the methods we measure. Part 1 (Kang-1) [RC, Kang-1] is a CPU
demonstrator with five methods; we quote its README:

> **1 – Classic.** … In practice, K is approximately 2.1 for this method.
> **2 – 3-way.** … In practice, K is approximately 1.6 for this method.
> **3 – Mirror.** … In practice, K is approximately 1.3 for this method. The main issue with this
> method is that the kangaroos loop continuously.
> **4 – SOTAv1 … 5 – SOTAv2 (new).** This method has same K=1.15 but uses only two groups of
> kangaroos and the symmetry of the elliptic curve. … I couldn't find any papers about this method,
> so let's assume that I invented it :)
> **6 – SOTA+.** … also uses cheap second point. When we calculate `NextPoint = PreviousPoint +
> JumpPoint` we can also quickly calculate `PreviousPoint - JumpPoint` because inversion is the
> same. … K is approximately 1.02 … Or you can pay 1MUL+1SQR and get K about 0.99 (preferable for
> GPU implementation). Or you can pay only (1MUL+1SQR)/4 and get K about 1.05.

Part 2 (Kang-2) [RC, Kang-2] documents the loop-handling problem and RC's "advanced" chain-preserving
solution (a second jump table with larger jumps used to exit small even-size loops without breaking
the chain, plus L2-loop handling), with RC's measured degradation under the naive method
(RANGE_BITS 56, DP 13: K = 2.60 naive vs 1.16 advanced). Part 3 (RCKangaroo) [RC, RCKangaroo] is
the GPU implementation: SOTA only, K = 1.15, with an empirical DP-overhead estimate that we validate
in §5.3.

The SOTA family is, to our knowledge, absent from the academic literature — which is a polite way
of saying that for two years the fastest publicly known attack on secp256k1 ranges existed only as
a GitHub repository and a README containing a smiley. The Bitcointalk research
thread [RC, thread] contains community verification of the constants via independent runs of RC's
code, but no statistical study, no mechanism analysis, and no independent reimplementation of the
walk statistics existed before this work.

### 2.3 Notation

W = 2^r is the interval width; √W = 2^(r/2). h = number of kangaroos ("herd size"), h/3 tame and
2h/3 wild in the SOTA family. d = DP bits; a DP is a point whose x-coordinate is 0 mod 2^d. M = 2^m
is the jump-table size; jump j_i ∈ [2^(r/2+3), 2^(r/2+4)) (mean 1.5·2^(r/2+3) = 12√W), even for all
SOTA-family methods. DPK = 1.15·√W/(h·2^d) is RC's "ideal DPs per kangaroo" load parameter. K =
(expected total group operations)/√W; all K quoted as mean ± 95% CI over independent trials
(uniform random k per trial, fresh jump tables per program run).

---

## 3 The SOTA family

### 3.1 Specification

We reverse-engineered and re-specified the method from Kang-1's `ExpKangDlg.cpp`
(`thr_proc_sota`, `thr_proc_sota_plus`) and mirrored it in our simulator (`kangsim_final.c`, §4).
The following is the exact semantics (method "SOTAv2" of Kang-1, which is what RCKangaroo
implements):

* **Target shift.** The DLP instance is shifted: Q' = Q + (W/32)·G, k' = k + W/32. All collision
  algebra targets k'. (RC: "add x32 just to improve K for range edges".)
* **Herds.** One third of the kangaroos are *tame*: displacement cp uniform in [0, W/32) (the
  *tame range* is only W/32 of the interval!). Two thirds are *wild*: start displacement cp uniform
  *even* in [0, 34·W/32) = [0, 1.0625·W), placed at −Q' (so the walked exponent is E = cp − k').
* **Walk.** At point P with x-coordinate x: jump index i = x mod M; sign s = +1 if y(P) is even
  else −1; P ← P + s·J_i; displacement ← displacement ± j_i. The sign is *antisymmetric* under
  point negation (y(−P) = −y(P) flips parity), and the index is *negation-invariant* (x(−P) = x(P)):
  the two properties together define the "adaptive-sign" walk.
* **DP handling.** If x' mod 2^d = 0, the triple (x', role, cp) is inserted into a shared table.
  On a key match between records with roles o₁, o₂ ∈ {0 (tame), −1 (wild)} and displacements
  c₁, c₂, both collision channels are tested:
  E₁ = E₂ ⇒ k' = (c₁ − c₂)/(o₂ − o₁) (o₁ ≠ o₂), and E₁ = −E₂ ⇒ k' = −(c₁ + c₂)/(o₁ + o₂).
  Same-role tame–tame matches are discarded; wild–wild matches with equal displacement are discarded.
* **Loop handling.** Kang-1 default: keep the last 16 displacement values per kangaroo; if the
  current displacement reappears (or 20·2^d steps pass without a new DP), *restart* the kangaroo at
  a fresh random start (cost 0 in the op count, as in Kang-1). This is the "simple" rule. Kang-2's
  advanced rule replaces the restart by a jump from a second table of much larger jumps, which
  preserves the chain (another kangaroo reaching the same chain point follows the same escape). Our
  simulator implements the simple rule, and an intermediate "escape" rule (`-esc`) that pre-computes
  the next step and diverts through a second table when it would exactly undo the current step
  (L1S2 bounce); §5.4 quantifies the gap between all three.
* **SOTA+** (Kang-1 `thr_proc_sota_plus`): at each step, after computing the main candidate
  P + s·J_i, the code computes the alternative candidate P − s·J_i *sharing the same inversion*
  (Kang-1's `AddPointsAndGetInv`), reads the low bits of the alternative's x-coordinate, and
  possibly switches:

  - **v0 ("free")** [Kang-1 first commented block]: if the main candidate's x is odd, compute the
    alternative; switch iff the alternative's x is even.
  - **v1 ("lite", pay (1MUL+1SQR)/4)**: if the main candidate's x ≡ 3 (mod 4), compute the
    alternative; switch iff the alternative's x ≢ 3 (mod 4).
  - **v2 ("GPU", pay 1MUL+1SQR)**: always compute the alternative; switch iff its x mod 4 is
    smaller than the main candidate's x mod 4.

  In all three variants the second point is *not counted as an operation* in Kang-1's op counter;
  the README prices it separately as (1MUL+1SQR)/4, (1MUL+1SQR) or free. §7.2 prices it honestly.

### 3.2 The walk is confined: the saturation band

The adaptive-sign walk is *not* a random walk with independent increments. The sign s at each
position is a deterministic function of the canonical position, and consecutive signed jumps are
strongly anti-correlated: in our EC and Z/p walk diagnostics the per-step sign-flip rate is
0.50–0.60 (a fair independent coin gives 0.5, but the anti-correlation extends to the jump
magnitudes as well). The consequence is striking enough that we re-checked it three separate ways,
and as far as we know it was undocumented in the community literature before this work:

**Empirical fact 3.1 (confinement).** The SOTA walk's displacement |dist| grows only for the first
~10^3–10^4 steps and then saturates at a *band* B(M) of width a few hundred √W, independently of
step count thereafter. Measured in the Z/p model at r = 31 (jumps in [8,16)·√W, mean 12√W):

| M (jump table) | mean \|dist\| after 10^3 steps | after 10^4 | after 10^5 | after 10^6 |
|---|---|---|---|---|
| 2^6  | 67√W  | 74√W  | 78√W  | 56√W  |
| 2^8  | 119√W | 124√W | 136√W | 179√W |
| 2^10 | 187√W | 233√W | 268√W | 297√W |
| 2^12 | 208√W | 366√W | 341√W | 335√W |
| 2^14 (default) | 191√W | 489√W | 377√W | 352√W |
| 2^16 | 201√W | 413√W | 507√W | 406√W |

(mean over 40 independent walks; max|dist| runs ≈ 1.5–1.9× the mean). The band grows sublinearly in
M — a power-law fit gives B ∝ M^0.4–0.5 — and is insensitive to the step count beyond 10^4. For
reference, an unbiased sign process would give a diffusive √t growth: after 10^6 steps the rms
displacement would be ≈ 12√W·10^3 = 1.2·10^4√W, four orders of magnitude larger than observed.

**Consequences.** (a) *Start ranges can be tiny.* A kangaroo explores only a band around its
start, so the herd must be pre-positioned across the interval rather than allowed to diffuse:
this is why tame range W/32 and wild range 1.0625·W suffice, and why herd geometry (§5.5), not
walk length, governs the constant. (b) *Loops are inevitable at scale.* Beyond saturation every
step revisits territory; small even-size loops (size 2 above all) occur at a rate set by M alone
(RC reports L1S2 ≈ 1/1024 jumps at M = 512) and accumulate linearly — the mechanism behind §5.4's
fragility results and RC's Kang-2 observation that naive loop handling degrades K at high ranges.
(c) *Uniform DP density comes only from pre-positioning*, which is why the DP-overhead accounting
(§5.3) transfers across ranges only when the herd scales correctly.

### 3.3 Stationary distribution and the selection-rule model

**Selection classes.** The walk state relevant to the oracles is the canonical x-coordinate
(the EC x-coordinate is negation-invariant). Its low bits supply, from the *same* bit-string: the
jump index (bits 0…m−1), the DP test (bits 0…d−1, d ≤ m), and the SOTA+ branch tag (bits 0–1).
Call the pair (tag, DP-bit-pattern) a *selection class*. Absent a selection rule the class of a
freshly visited point is uniform: DP rate exactly 1/2^d (measured: 1/32.0 at d = 5; the hash-oracle
DP distribution check in §4.2.3 gives 1/32.06 and 1/909091 for 20-bit patterns vs 1/1048576 nominal).

**The SOTA+ rules bias the classes.** Treating successive steps' candidate tags as i.i.d. uniform
on {0,1,2,3} (a Null model we validate below), the rule-induced distribution π over tags of
*visited* states is:

| rule | switch condition | π(tag=0) | π(tag=1) | π(tag=2) | π(tag=3) | Σπ² (Herfindahl H) | predicted DP rate | measured |
|---|---|---|---|---|---|---|---|---|
| SOTA (no selection) | — | 1/4 | 1/4 | 1/4 | 1/4 | 0.250000 | 1/32.00 | 1/32.0 |
| SOTA+ v0 | main odd → alt if alt even | 3/8 | 1/8 | 3/8 | 1/8 | 0.312500 | 1/21.33 | 1/21.3 |
| SOTA+ v1 | main ≡ 3 (mod 4) → alt if alt ≢ 3 | 5/16 | 5/16 | 5/16 | 1/16 | 0.296875 | 1/25.60 | 1/25.6 |
| SOTA+ v2 | always min-tag | 7/16 | 5/16 | 3/16 | 1/16 | 0.328125 | 1/18.29 | 1/18.3 |
| SOTA + mirror add-on | second unconditioned trail | — | — | — | — | — | 1/16.00 | 1/16.0 |
| SOTA+ v2 + mirror | | — | — | — | — | — | 11/128 = 1/11.64 | 1/11.6 |

Every entry of the last two columns is an exact combinatorial identity of the Null model (DP ⟺
low d bits zero ⟹ tag = 0; the unshared d−2 bits remain uniform), and all six measured rates match
the identities to ≤ 0.4% — the resolution of the measurement.

**Why this lowers K.** Collisions are detected only at stored DPs, so the collision-relevant
distribution is the walk's visit intensity restricted to DP states. A rule that preferentially
selects low-tag states both (i) raises the rate at which steps land on DPs (the identities above)
and (ii) concentrates visit intensity on low-bit states — a concentration that §3.4 shows operates
even when the tag is *not* part of the DP pattern, so the two effects are separable, not one. The empirical scaling law we observe
across the whole family is

**Law 3.2 (Herfindahl scaling).** K_ideal(rule) ≈ K_ideal(SOTA) · √(H_unif / H(rule)),
with H_unif = 0.25, H as in the table, K_ideal(SOTA) = 1.15.

| rule | Law 3.2 prediction | RC published | measured (2^40, dp5) | measured (2^31, dp5) |
|---|---|---|---|---|
| SOTA | 1.1500 | 1.15 | 1.1537 ± 0.0195 | 1.4012 ± 0.0091 |
| SOTA+ v0 | 1.0286 | 1.02 | 1.0242 ± 0.0171 | 1.1968 ± 0.0080 |
| SOTA+ v1 | 1.0554 | 1.05 | 1.0548 ± 0.0180 | 1.2667 ± 0.0083 |
| SOTA+ v2 | 1.0038 | 0.99 | 1.0093 ± 0.0170 | 1.1522 ± 0.0079 |

At 2^31 the DP overhead is large (§5.3), so the raw measurements sit above the ideal constants; adding
RC's DP-overhead formula driven by the *effective* DPK = DPK·(32/measured-DP-rate) reproduces the
measurements to ≤ 1%: predicted 1.1964 / 1.2567 / 1.1473 vs measured 1.1968 / 1.2667 / 1.1522
(residuals +0.03%, +0.8%, +0.4%). At 2^40 (dp 5, DPK = 73.6, overhead < 1%) the raw agreement is
already ≤ 0.8%.

**Honesty about Law 3.2.** We do not have a first-principles derivation of the exponent. The naive
birthday computation with stationary visit measures over-predicts the gain (it would give
K/K_SOTA ≈ 1/√(4H) ≈ 0.57 for v2, vs 0.87 measured), because stationary occupancy weights revisits
whereas the birthday process samples fresh draws; the √(H) law is a phenomenological fit with six
in-range verifications and two out-of-sample validations (the EC cross-checks of §5.6 and the
mirror combinations). Its sharpest structural test — re-keying the tag to an independent hash,
which leaves H unchanged — is passed: §3.4 shows the law holds in the control world to +1.0%,
which is why we present the concentration and DP-synergy components as separate, individually
validated effects rather than one mechanism.

### 3.4 Control experiment: independent-hash branch tag

We modified the simulator so that the SOTA+ branch tag is read from an independent hash of the
canonical position (same distribution, different key), leaving jump index and DP test untouched
(`kangsim6.c -it`). The control separates the two candidate explanations of the SOTA+ gain: a
*concentration* effect (the walk concentrates on a tag-selected quarter of the space, whichever
hash defines it) and a *DP-synergy* effect (the selected quarter is the DP quarter, so the DP rate
rises). If concentration alone drives the gain, K should improve in the control too; if DP-synergy
drives it, the gain should vanish and the DP rate must return to 1/2^d.

| configuration (2^31, dp 5, h = 512) | trials | K (mean ± 95% CI) | DP rate | ideal-equivalent K |
|---|---|---|---|---|
| SOTA baseline (no selection) | 20 000 | 1.4012 ± 0.0091 | 1/32.0 | 1.15 |
| SOTA+ v2, shared tag | 20 000 | 1.1522 ± 0.0079 | 1/18.3 | 1.004 |
| SOTA+ v2, independent tag | 12 000 | **1.2653 ± 0.0105** | **1/32.0** | 1.017 |
| SOTA+ v0, independent tag | 12 000 | 1.2626 ± 0.0104 | 1/32.0 | 1.014 |
| (Kang-1 config, 2^40: shared vs independent, v2) | 4 000 + 4 000 | 1.0093 ± 0.0170 vs **1.0126 ± 0.0166** | 1/18.3 vs 1/32.0 | — |

Three findings. (i) The DP-rate elevation is *entirely* a bit-sharing effect: with an independent
tag the DP rate returns to exactly 1/32.0 under both rules. (ii) The concentration gain survives:
at 2^31 the independent-tag K (1.2653) is 9.7% below SOTA, and Law 3.2 plus RC's overhead formula
at the *uniform* DP rate predicts 1.2525 for it (+1.0% residual) — the same law that predicts the
shared-tag run to +0.4%. The gain therefore does **not** require bit-sharing. (iii) What bit-sharing
buys is precisely the DP-rate elevation, and its value is the DP-overhead share of K: measured
shared-vs-independent gap 0.113 at 2^31/dp5 vs 0.105 predicted from the overhead formula; at the
Kang-1 configuration (dp 5, DPK = 73.6, overhead < 1%) the gap is 0.003 ± 0.024 — statistically
zero. In sparse-DP regimes the gap grows with the overhead term (predicted 0.14 at 2^31/dp12),
never dominating.

**A methodological warning we inflicted on ourselves.** A 200-trial pilot of the independent-tag
run gave K = 1.3264 ± 0.0847, which reads as "the gain mostly vanishes" — the conclusion we
originally drafted. The 12 000-trial run gives 1.2653 ± 0.0105 and reverses it. Per-trial K is
heavy-tailed (§5.1); pilot-scale samples in this problem class do not support qualitative
conclusions about few-percent effects. We publish the pilot number next to the real one in the
hope that the next person tempted by n = 200 finds this paragraph first.

Revised conclusion: **the SOTA+ advantage decomposes into a selection-induced concentration of the
walk (present for any position-derived tag; ≈ 13% of K at ideal) and a tag⊗DP bit-sharing synergy
that raises the DP rate to exactly predictable values and is worth exactly the DP-overhead share
it removes.** Implementations may substitute an independent tag source only if they do not care
about DP density; the concentration half of the benefit is retained.

---

## 4 Experimental setup

### 4.1 The Z/p hash-oracle model

Our primary instrument (`kangsim_final.c`, ~420 lines of C, Appendix A) models the group as the
additive group of the prime field Z/p with p = 2^61 − 1, representing a kangaroo by its *true
exponent* E (an integer with |E| ≪ p, so no wraparound is exercised) and its displacement cp; the
group operation is exact. Negation is E → −E. All three oracles that the real implementation derives
from the x-coordinate are modeled as hash bits of the *canonical form* canon(E) = min(E, p−E),
which is ±-invariant exactly like the EC x-coordinate:

* jump index i = H(canon E) mod M, H = mix64 (splitmix64 finalizer), a fixed keyed permutation;
* DP test: (H(canon E) & (2^d − 1)) = 0 — the same hash, hence the same low-bit structure as the
  index, as on the curve;
* branch tag: H(canon E) & 3 — again the same hash.

The y-parity jump-sign bit is modeled as an *antisymmetric* hash bit: s(E) = (H₂(canon E) & 1)
⊕ [E ≠ canon E], so that s(−E) = 1 − s(E) — the defining property of the EC y-parity. The op
counter charges exactly 1 per point addition (one jump step of one kangaroo); restarts cost 0, as
in Kang-1; the SOTA+ alternative point costs W_ALT/1000 op when the `-w` pricing flag is set
(default 0, matching Kang-1's accounting).

**Collision algebra** is exactly that of §3.1: a stored record is (canon x, role, cp) with
E = cp + o·k′, o ∈ {0 (tame), −1 (wild)}, k′ = k + W/32 for the SOTA family and k′ = k for
classic/3-way; both the E₁ = E₂ and E₁ = −E₂ channels are solved for k′ and compared against the
planted target. The per-method candidate sets were checked by hand against Kang-1's
`Collision_SOTA` and the classic/3-way procedures (Kang-1 checks only the same-direction channel
for those methods; we mirror that).

The DB is an open-addressing table with generation tags, sized to keep load below ~0.5 for every
configuration (2^21–2^23 entries); false collisions from hash-table aliasing cannot occur by
construction (keys are exact canonical values, not truncated), and DP *value* collisions are the
real birthday events the experiment measures.

### 4.2 Fidelity checks against real secp256k1

Three instruments connect the model to the real curve:

1. **`walktest.c` — single-walk statistics on real secp256k1 (OpenSSL) vs the model.** Identical
   walk rule (index = x mod M, sign = y parity antisymmetric, jumps even in [2^(r/2+3), 2^(r/2+4))),
   10^5 steps × 20 trials at r = 32: EC mean |dist| 3.43·10^7 vs model 4.01·10^7 (max |dist|
   1.54·10^8 vs 1.66·10^8); sign-flip rate 0.597 (EC) vs 0.498 (model) per step; both far from the
   diffusive expectation 2.49·10^8. Both walks are confined, with the model ~17% wider on this
   sample; §4.4 treats this as the model's main quantitative deviation.

2. **`eckang.c` — full DLP solves on real secp256k1.** A line-by-line EC re-implementation of the
   SOTA and SOTA+ methods using OpenSSL affine arithmetic, with the SOTA+ step computing *both*
   branches from one inversion of (x_J − x_P), exactly as RC's `AddPointsAndGetInv` does. Runs at
   r = 31 (exactly matching the Z/p n = 32 runs' geometry, including minjump = 2^18 and the W/32
   shift):

   | EC run (r = 31, h = 512) | K (real secp256k1) | paired Z/p measurement | σ of difference |
   |---|---|---|---|
   | SOTA, dp 5, 150 trials | 1.4162 | 1.4012 ± 0.0091 | 0.28σ |
   | SOTA+ v2, dp 5, 100 trials | 1.0420 | 1.1522 ± 0.0079 | **2.1σ** (see below) |
   | SOTA, dp 12, 30 trials | 6.4012 | 5.8106 ± 0.0472 | 1.2σ |

   (SEs use the Z/p per-trial standard deviation at the same configuration: 0.66 for SOTA dp5,
   0.53 for SOTA+ dp5, 2.64 for dp12.) The SOTA and dp12 comparisons validate the model. The
   SOTA+ point is 2.1σ *below* the model — and three further real-curve runs agree with each
   other (r = 32: 1.0512, 30 trials; 1.0395, 60 trials; r = 31: 1.0420), so the discrepancy is
   stable, not a single fluke. Two readings: a heavy-tail artifact of 100–190 total trials, or a
   genuine small extra benefit of the *algebraic* (non-random) low-bit oracle for the selection
   rule — the real curve relates consecutive positions' low bits through λ² = x′ + x + x_J, a
   structure our random oracle destroys, and a structure that could strengthen the DP-seeking
   dynamics of §3.3. We flag it as an open discrepancy; it does not affect any conclusion that
   rests on the Z/p instrument, and it makes our SOTA+ claims conservative (the real curve appears
   if anything slightly better than modeled).

3. **`hashtest.c` — oracle uniformity.** Over 10^7 canonical forms: DP-rate for a 5-bit mask
   1/32.06 (nominal 1/32) and for a 20-bit mask 1/909091 (nominal 1/1048576); the 20-bit estimator
   is within its Poisson 1σ (expected 9.5 hits, observed 11). The model oracle is uniform.

### 4.3 Configurations

Three interval widths, chosen to bracket RC's own published experiments:

* **n = 32 (r = 31):** 20 000 trials per configuration (12 000 for sweeps), the statistical
  backbone; h = 512 default, dp = 5 default.
* **n = 41 (r = 40):** 4 000 trials, the *exact* Kang-1 configuration (RANGE_BITS 40, DP_BITS 5,
  KANG_CNT 512, JMP_CNT 2^14, POINTS_CNT 1000-equivalent trial count).
* **n = 48 (r = 47):** 100–300 trials, the large-range fragility regime (Kang-2's RANGE_BITS 48
  row), with dp sweeps 12–24 and herd sweeps h ∈ {512, 2048, 8192, 32768}.

Sweeps: dp ∈ {4,6,8,10,12,14} (n=32) and {12,14,16,18,20,24} (n=48); h ∈ {64,128,512,2048,8192}
(n=32) and {512,2048,8192,32768} (n=48); jump-table size M ∈ {2^9, 2^11, 2^14}; jump-scale
minjump multiplier 2^(r/2+js) for js ∈ {2,3,4,5}; mean-jump sweep for the 1v1 ideal model
(§5.7). Every configuration runs to a cap of 200·√W (400·√W for 1v1) operations; trials reaching
the cap are *censored* and excluded from the mean, with the censoring count reported (censoring
biases the reported mean low; we flag every table entry with censoring > 1%).

Seeds: a fixed seed per program run (0xABCDEF0123456789) with fresh jump tables per run; trial
level randomness streams from a counter-based RNG. The jump table is a single fixed draw per run —
the K fluctuation attributable to the table draw is estimated in §4.4.

### 4.4 Caveats, stated explicitly

1. **The oracle is a hash, not EC arithmetic.** On the curve, the jump index is x mod 2^m of the
   *field element* x; consecutive walk positions' low bits are then related by the group law
   (x' = λ² − x − x_J), not by a random function. Our model replaces this by a random oracle keyed
   on the canonical exponent. The measured quantities that could detect the difference —
   single-walk displacement statistics (§4.2.1), sign-flip rate, DP-rate uniformity, and end-to-end
   K at matched configuration — agree to 0.3–17%; the single-walk band width is the weakest
   agreement (model ~17% wide on 20 trials). All K conclusions rest on the end-to-end checks.
2. **One jump table per run.** Jump magnitudes u_i are a fixed random draw; the Herfindahl model
   assumes u ⊥ tag in expectation. Within a tag-0 subclass of M/4 indices the realized mean of u²
   fluctuates by ~σ(u²)/(E[u²]·√(M/4)) ≈ 1.6% at M = 2^14; this propagates a ~0.8% seed-to-seed
   uncertainty into Law 3.2's predictions, of the same order as the observed residuals. The
   M = 2^9 runs (§5.4) show what a *pathologically* correlated table draw does at scale.
3. **Op accounting follows Kang-1.** Restarts are free; the SOTA+ second point is free unless `-w`
   is set. Both choices are RC's; §7.2 re-prices them. In particular our "K = 1.1537" for SOTA
   includes DP overhead at dp 5 (≈ 0.7%), and our "ideal-equivalent" columns are model estimates,
   not direct measurements.
4. **Censoring.** Trials hitting the op cap are dropped; where censoring exceeds 1% (dp14 at n=32:
   165/12000; dp24 at n=48: 48/60) the mean is biased low and we say so.
5. **Loop handling is ours, not RC's.** Our "simple" rule matches Kang-1's; our `-esc` rule is an
   approximation of Kang-2's advanced handling (it catches the L1S2 bounce but not general
   even-size loops or L2 loops). RC's published large-range numbers (K = 1.16 at RANGE_BITS 56)
   are therefore *not* directly comparable to our n = 48 numbers; we use our own runs only for
   *relative* statements about loop handling.
6. **No adversarial robustness.** The oracles are deterministic and public; a real implementer
   must also resist intentional or accidental non-randomness of x-coordinates (e.g. points chosen
   by an adversary, or DP-biased inputs). The SOTA+ selection rule increases the walk's dependence
   on low-bit structure, making this assumption load-bearing (§7.4).
7. **Memory is not priced.** K counts group operations; DB size (DPs stored) scales as
   1.15·√W/2^d entries and is identical across methods at fixed d except where noted (SOTA+
   elevated DP rates enlarge it proportionally; the mirror doubles it).

---

## 5 Results

### 5.1 Main table: method × interval width

K = (total group operations)/√W, mean ± 95% CI; median in parentheses; all at h = 512, M = 2^14,
minjump = 2^(r/2+3), RC op accounting.

| method | n = 32 (20 000 trials) | n = 41 (4 000 trials) | n = 48 (dp14; 300 tr.) |
|---|---|---|---|
| classic herd (Kang-1 "Classic") | 2.3846 ± 0.0157 (2.255) | 2.1113 ± 0.0348 (1.973) | 2.7695 ± 0.1463 (2.615) |
| 3-way | 1.9003 ± 0.0129 (1.779) | 1.6141 ± 0.0284 (1.471) | 2.1744 ± 0.1006 (2.098) |
| mirror (Kang-1 "Mirror", method 6) | 1.6324 ± 0.0105 (1.542) | 1.3508 ± 0.0268 (1.238) | 2.2834 ± 0.1882 (2.148) |
| SOTA | 1.4012 ± 0.0091 (1.320) | **1.1537 ± 0.0195** (1.077) | 1.9502 ± 0.1025 (1.846) |
| SOTA+ v0 | 1.1968 ± 0.0080 (1.130) | **1.0242 ± 0.0171** (0.949) | — |
| SOTA+ v1 | 1.2667 ± 0.0083 (1.196) | **1.0548 ± 0.0180** (0.983) | — |
| SOTA+ v2 | 1.1522 ± 0.0079 (1.075) | **1.0093 ± 0.0170** (0.940) | 1.5078 ± 0.0816 (1.490) |

Boldface: the exact Kang-1 configuration, where comparison with RC's published constants is
parameter-free. The n = 32 and n = 48 columns include the DP overhead of their configurations
(§5.3) and, at n = 48, the loop-handling fragility (§5.4); neither is part of the published
constants. Distribution shape: per-trial K is right-skewed everywhere (median < mean by 5–45%),
extremely so for the 1v1 ideal model (§5.7) and for censored large-dp runs.

The three legacy methods reproduce RC's practice numbers at n = 41 exactly (2.11 vs "≈2.1",
1.61 vs "≈1.6"), and the mirror method lands at 1.35 vs "≈1.3". At n = 48 every method degrades
(§5.4–5.5); at n = 32 SOTA matches the DP-formula prediction to +0.2% while the two legacy
methods run +6.5–6.8% over an extrapolation of that formula — unsurprising, since the formula's
constants were fitted on SOTA data, but a reminder that it is not method-generic (§5.3).

### 5.2 The SOTA+ variants vs the stationary model

Covered in §3.3 (Law 3.2 table): all four ideal constants reproduced within ±0.015 of RC's
published values by the model and within ±0.02 by direct measurement; the SOTA+ ordering
v2 < v0 < v1 is reproduced, including the counterintuitive fact that v1 (which pays more for its
second point than v0) has a *worse* ideal constant — its weaker switch condition (only main ≡ 3
mod 4) buys less bias than v0's, and v2's always-compare rule buys the most. The DP-rate identity
column is the sharpest test: all six predictions match to ≤ 0.4%.

### 5.3 DP-overhead formula: validation and validity domain

RC's RCKangaroo estimates the achievable K at load DPK = 1.15·√W/(h·2^d) as

  K = 1.15 + (0.07 + 0.76/√DPK) / (1 + 0.30·DPK).   (RC formula)

Our dp and herd sweeps give an independent test over DPK ∈ [0.002, 74] — more than four orders of
magnitude. Residual = (measured − formula)/formula.

| config (all SOTA, h = 512 unless noted) | DPK | formula | measured | residual | notes |
|---|---|---|---|---|---|
| n=32, dp 4 | 6.505 | 1.2747 | 1.2658 ± 0.0114 | −0.7% | |
| n=32, dp 6 | 1.626 | 1.5976 | 1.6153 ± 0.0127 | +1.1% | |
| n=32, dp 8 | 0.407 | 2.2747 | 2.3119 ± 0.0167 | +1.6% | |
| n=32, dp 10 | 0.102 | 3.5312 | 3.5579 ± 0.0260 | +0.8% | |
| n=32, dp 12 | 0.025 | 5.9510 | 5.8106 ± 0.0472 | −2.4% | |
| n=32, dp 14 | 0.006 | 10.7369 | 11.9946 ± 0.1699 | +11.7% (cens. 165/12000; true value higher) | beyond formula's domain |
| n=48, dp 12 (esc) | 6.505 | 1.2747 | 1.4397 ± 0.0959 | +12.9% | herd geometry (§5.5) |
| n=48, dp 14 (esc) | 1.626 | 1.5976 | 1.7601 ± 0.1045 | +10.2% | herd geometry |
| n=48, dp 16 (esc) | 0.407 | 2.2747 | 2.2652 ± 0.1582 | −0.4% | |
| n=48, dp 18 (esc) | 0.102 | 3.5312 | 4.0484 ± 0.3290 | +14.6% | onset of pathology |
| n=48, dp 20 (simple) | 0.025 | 5.9510 | 10.657 ± 1.5753 | +79% | restart-dominated |
| n=48, dp 24 (simple) | 0.002 | 20.2812 | 19.4077 ± 4.5030 | −4.3% | **48/60 trials censored — configuration is dead** |
| n=48, h = 2048, dp 12 | 1.626 | 1.5976 | 1.6037 ± 0.0774 | +0.4% | |
| n=48, h = 2048, dp 14 (esc) | 0.407 | 2.2747 | 2.2832 ± 0.1245 | +0.4% | |
| n=48, h = 8192, dp 11 | 0.813 | 1.8838 | 1.9150 ± 0.0830 | +1.7% | |
| n=48, h = 8192, dp 12 (esc) | 0.407 | 2.2747 | 2.3105 ± 0.1069 | +1.6% | |
| n=48, h = 32768, dp 16 (esc) | 0.006 | 10.7369 | 8.4238 ± 0.3572 | **−21.5%** | formula over-penalizes |
| n=48, h = 512, dp 5 (=Kang-1) | 73.60 | 1.1569 | 1.1537 ± 0.0195 | −0.3% | the calibration point |
| n=32, h = 512, dp 5 | 3.253 | 1.3987 | 1.4012 ± 0.0091 | +0.2% | |
| n=32, h = 64, dp 5 | 26.02 | 1.1749 | 1.3379 ± 0.0155 | **+13.9%** | herd geometry |
| n=32, h = 128, dp 5 | 13.01 | 1.2072 | 1.2930 ± 0.0133 | +7.1% | herd geometry |
| n=32, h = 2048, dp 5 | 0.813 | 1.8838 | 1.8165 ± 0.0136 | −3.6% | |
| n=32, h = 8192, dp 5 | 0.203 | 2.8047 | 2.5621 ± 0.0148 | −8.6% | |

**Summary.** The formula is accurate to ≤ 2.5% across its designed domain: at the calibration herd
(h = 512) for r ∈ {31, 40} over DPK ∈ [0.025, 74], and at r = 47 for h ≥ 2048. It fails in three
regimes, all *outside* the domain it was fitted on: (i) small herds at wide intervals (+7–14%,
geometric, §5.5); (ii) extreme low DPK at wide intervals with naive loop handling (dp ≥ 18 at
n = 48: +15% to +79%, loop/DP pathology, §5.4); (iii) very large herds at very low DPK (−21.5% at
h = 32768: the formula's DPK-only parametrization cannot see that a huge herd's short walks do not
pay the predicted overhead). For planning purposes the formula is excellent; as a law it is a
two-parameter fit around (r, h) = (40, 512).

### 5.4 Fragility I: loop handling

At n = 48 (r = 47), h = 512, M = 2^14, the walk band (≈ 2^32.2) is ~90× narrower than the wild
start spacing (2^38.3); each kangaroo walks 10^4–10^5 steps inside a saturated band, and loops
dominate behavior. A kangaroo that has saturated its band is not exploring; it is pacing its cage —
and the published constant 1.15 quietly becomes 1.95 unless the cage is handled. Measured at dp 14
(300 trials, simple handling; 200 trials, escape handling):

| loop handling | K | restarts/kang | formula | ideal |
|---|---|---|---|---|
| simple (Kang-1 default: restart on revisit) | 1.9502 ± 0.1025 | 1.32 | 1.5976 | 1.15 |
| escape (L1S2 bounce diverted through second table) | 1.7601 ± 0.1045 | 0.002 | 1.5976 | 1.15 |

The naive rule costs +10.8% over the escape rule at dp 14; the gap widens as DP gets sparser
(simple vs escape): dp 12: 1.4416 vs 1.4397 (+0.1%); dp 16: 2.8218 vs 2.2652 (**+24.6%**); dp 18:
4.9955 vs 4.0484 (**+23.4%**); dp 20: 10.657 (simple only; escape not run); dp 24: 48/60 trials
never solve. The direction of the dp-dependence is informative: at *high* DP density the DP rate
itself resets the loop detectors (a DP insert clears the 16-deep history), so simple handling is
fine (dp 12: +0.1%); at *low* DP density the detector must do the work and the restart's cost
(discarding the kangaroo's accumulated chain) is paid between every few DPs.

Two further fragility datapoints at n = 48: the *jump-table size* M = 2^9 gives K = 4.6974 ± 0.3035
(+194% over formula; 103.5 restarts/kang — the L1S2 loop rate scales as ~1/M and the band is
narrow), while M = 2^11 at n = 32 is harmless (1.4179 ± 0.0118, +1.4%); and the legacy *mirror*
method, whose walks loop "continuously" in RC's words, shows the worst degradation of any method
(2.2834 vs formula 1.5976, +42.9%, restarts/kang 1.56).

RC's own published comparison (Kang-2 README: RANGE_BITS 56, DP 13: K = 2.60 simple vs 1.16
advanced) is consistent with our measurements: both show that the *published* SOTA constant 1.15
is a statement about the chain-preserving loop handler, not about the walk in isolation. Any
independent implementation that restarts on revisit will land near our 1.95, not near 1.15, at
large ranges.

### 5.5 Fragility II: herd-size scaling and the spacing/band law

At fixed method and dp, the herd size h changes K in ways the DPK-only formula cannot express.
Measured at n = 32, dp 5 (formula value from DPK):

| h | DPK | formula | measured | residual | wild start spacing | spacing/band |
|---|---|---|---|---|---|---|
| 64 | 26.02 | 1.1749 | 1.3379 ± 0.0155 | +13.9% | 2^25.7 | ≈ 2.4 |
| 128 | 13.01 | 1.2072 | 1.2930 ± 0.0133 | +7.1% | 2^24.7 | ≈ 1.2 |
| 512 | 3.253 | 1.3987 | 1.4012 ± 0.0091 | +0.2% | 2^22.7 | ≈ 0.3 |
| 2048 | 0.813 | 1.8838 | 1.8165 ± 0.0136 | −3.6% | 2^20.7 | ≈ 0.07 |
| 8192 | 0.203 | 2.8047 | 2.5621 ± 0.0148 | −8.6% | 2^18.6 | ≈ 0.02 |

(band B ≈ 2^24.4 at r = 31, M = 2^14, from §3.2; wild spacing = 1.0625·W/(2h/3)). Within this
sweep the residual is monotone in the spacing/band ratio: the formula over-penalizes DP overhead
when the herd over-tiles the interval (each kangaroo's short walk still lands in someone's band),
and under-penalizes the *geometry* when the herd under-tiles it (gaps between wild bands must be
bridged by the walk's own excursion, which is bounded by the band). The same monotone trend
appears within the n = 48 sweep (§5.3): h = 512 has spacing/band ≈ 92 (spacing 2^38.3 vs band
2^32.2) and shows +10–13% at dp ≤ 14, h = 2048 (≈ 23) shows +0.4%, h = 8192 (≈ 6) shows +1.6% —
and h = 32768 (≈ 1.5, over-tiled at this dp) flips the sign: −21.5%. Across ranges the ratio is
not the only variable (the n = 48 dp-16 point nets −0.4% because the formula's overhead term is
slightly conservative at that geometry); the safe statement is monotonicity *within* a range, plus
the failure mode below.

**Failure mode.** At r = 47, h = 512, dp 24 the run does not merely degrade — it fails: 48 of 60
trials hit the 200√W cap without a single useful collision (the 12 that solved took
K = 19.41 ± 4.50). The mechanism is arithmetic: in the 12 trials that solved, the herd had stored
only ≈ 14 DPs when the collision landed, and even a full 200√W budget stores only ≈ 140 DPs — far
below any birthday requirement — so in this configuration collisions are geometric-luck events,
not birthday events: the herd neither tiles the interval (spacing/band ≈ 92) nor generates enough
DPs per unit time to compensate, and 48 of 60 trials run out of luck.
Practical reading: *h and d must be chosen together with r*; RC's formula embeds this only through
DPK, which is blind to the spacing/band ratio that actually governs the failure.

### 5.6 EC cross-validation

The paired-geometry real-curve runs are detailed in §4.2.2; in brief, real-curve SOTA agrees with
the model at r = 31 (dp5: 1.4162 vs 1.4012 ± 0.0091, 0.28σ; dp12: 6.4012 vs 5.8106 ± 0.0472, 1.2σ)
and earlier r = 32 runs agree at 0.8σ (SOTA) and 0.9σ (SOTA+). The one outlier is SOTA+ dp5
(1.0420 vs 1.1522 ± 0.0079, 2.1σ below, stable across three independent runs) — an open discrepancy
(§4.2.2, §7.4b) that makes our SOTA+ claims conservative. What matters for every conclusion below
is that the EC instrument confirms the model's constants to a few percent.

### 5.7 Robustness to jump-scale, table size, and the ideal 1v1 model

* **Jump scale.** minjump = 2^(r/2+js) for js ∈ {2,3,4,5} (mean jump 4√W–32√W): at n = 32,
  K = 1.4164 / 1.4012 / 1.4152 for js = 2/3/4 — insensitive to the jump scale over a 4× range,
  the constant being carried by the geometry and the selection structure rather than the jump
  magnitude. At n = 48 the scale matters only through loop exposure: with escape handling js 3/4
  give 1.7601 / 1.7298 (−1.7%), and with simple handling js 3/4/5 give 1.9502 / 1.8035 / 1.7832 —
  larger jumps mean fewer band revisits per unit distance and hence less restart damage, an
  effect that vanishes once loops are handled properly.
* **Jump-table size.** M = 2^9/2^11/2^14 at n = 32: 1.4272 / 1.4179 / 1.4012 (M = 2^9 costs +2%);
  at n = 48, M = 2^9 is a catastrophe (4.6974, +194%; §5.4) — the table must scale with the walk
  length.
* **1v1 ideal model (no DP, no herd).** The textbook 1-tame-1-walk every-point-check model gives,
  at n = 32 (20 000 trials): mean jump 0.5√W: K = 1.8916 ± 0.0138; 1√W: 2.4854 ± 0.0199; 2√W:
  4.2262 ± 0.0456; 3√W: 3.5121 ± 0.0277; 4√W: 11.1402 ± 0.1207 (median 19.38 — this run's mean is
  tail-dominated). At n = 41: 0.5√W: 2.3194 ± 0.0617; 1√W: 4.0766 ± 0.2047 (800 trials);
  3√W: 3.3738 ± 0.0776. Two readings: (i) the every-point model has no DP overhead, yet its
  constant is *worse* than the DP-based herd methods (2.3–4 vs 1.15–2.4) — the herd + DP structure
  is not an overhead, it is the algorithm; (ii) the per-trial cost distribution is heavy-tailed
  (median ≪ mean at jm ≥ 1), a warning against small-sample K estimates in this problem class.

---

## 6 Negative results

### 6.1 The correlated-mirror second sample

**Construction.** Each kangaroo additionally advances a "mirror trail" Y = c − X (same jumps,
negated; displacement tracked accordingly), DP-checked and inserted into the same DB, at a priced
cost of W_ALT/1000 op per step (0 = Kang-1-style free assumption). The algebra gives the mirror
trail its own valid collision channels (mirror-tame × mirror-wild and main × mirror), so this is
a candidate "free second sample per operation".

**Measured (h = 512, dp 5, M = 2^14).**

| config | K (second sample free) | K_weighted at 0.3 op/step | baseline | gain (free) |
|---|---|---|---|---|
| SOTA + mirror, n=32 (20 000 tr.) | 1.3644 ± 0.0089 | — | 1.4012 ± 0.0091 | **2.6% ± 1.3% (2.9σ)** |
| SOTA + mirror, n=41 (4 000 tr.) | 1.1262 ± 0.0186 | 1.4586 (priced run) | 1.1537 ± 0.0195 | 2.4% ± 2.4% (1.0σ) |
| SOTA+ v2 + mirror, n=32 (20 000 tr.) | 1.1109 ± 0.0076 | — | 1.1522 ± 0.0079 | 3.6% ± 1.3% (3.8σ) |
| SOTA+ v2 + mirror, n=41 (4 000 tr.) | 0.9724 ± 0.0165 | 1.2641 (priced run) | 1.0093 ± 0.0170 | 3.7% ± 2.3% (1.6σ) |
| SOTA + mirror, n=48 dp14 (150 tr.) | 1.7977 ± 0.1402 | — | 1.7601 ± 0.1045 (esc) | −2.1% ± 10% (n.s.) |

**Findings.** (i) The statistically significant gains are ≤ 3.6% at *zero* priced cost — far below
the √2 ≈ 41% a second independent walk would deliver. (ii) The reason is visible in the match
statistics: the mirror channel's matches are almost entirely *spurious* — at n = 41 the
"other"-category matches (algebraically solvable candidates that do not equal the target) number
57–80 out of ≈830k matches in the two mirror add-on runs, and in the pure mirror method 304k out
of 404k;
the best out-of-window candidate reaches |k′ − candidate| ≈ 21√W at n = 41. The mirror trail's
displacement is a deterministic reflection of the main trail's displacement plus a random offset
drawn from the tame-range scale (W/32 in our parameterization); collision candidates from the
mirror channels are therefore shifted by ~W/32…W — outside the collision window — and function as
DB pollution, not as information. (iii) What the mirror actually buys is DP density: the second
trail doubles the per-step DP checks (measured dp rate 1/16 = 2/32 exactly; 11/128 for the SOTA+
combination, again exact), i.e. it trades DB size for a few percent of K. (iv) Priced at even
0.3 op/step — far cheaper than any real implementation of a second point — every mirror variant
loses to its baseline outright (1.4586 vs 1.1537; 1.2641 vs 1.1537). Break-even is
W_ALT ≈ 24/1000 op/step for SOTA+mirror at n=41: **money-losing at any physically realizable
price**. (v) A subtle implementation coupling: mirror inserts reset the loop detector of the
inserting kangaroo, which alone improves K_main by ~2% at n = 41 (restarts/kang 0.078 vs 0.085);
implementers evaluating "mirror-like" tricks should control for this side effect. The mirror, in
short, is an excellent way to double your database and a poor way to halve your work.

### 6.2 The λ-endomorphism containment obstruction for 1-D walks

secp256k1 has an efficient endomorphism: the map ψ(x, y) = (β·x mod p, y), with β a cube root of
unity mod p, satisfies ψ(P) = [λ]P for a cube root of unity λ mod n (λ³ ≡ 1 mod n, λ ≢ 1).
Equivalence-class methods (Galbraith–Ruprai [G-R 2010]) accelerate interval DLP by walking on the
orbits {±P, ±ψP, ±ψ²P} of size 6 — the walk only needs to hit a class-mate of the target — which
is the mechanism behind their ≈ 1.36√N constant in groups with fast inversion.

The obstruction for a *1-dimensional* walk on secp256k1: a kangaroo walk stores displacements
c and recovers k from a collision by integer arithmetic in the exponent domain. For the class
method to function, every stored exponent must lie in a fixed interval of width ~W (otherwise the
algebra of the collision returns candidates outside the search window and the method degenerates
to the mirror pathology of §6.1). The λ-orbit of an exponent k is {k, λk mod n, λ²k mod n} (up to
sign): multiplication by λ mod n acts as a *fixed* multiplier ρ ≡ λ (as an element of Z_n), and
for k uniform in [0, W) with W ≪ n, ρ·k mod n lands in [0, W) with probability ~W/n ≈ 0 (and when
it does, it is not controllably placeable). No interval of width W ≪ n is invariant under
multiplication by a unit ρ ≢ ±1. Hence a 1-D walk cannot contain the λ-class representatives, and
the class speedup is unrealizable in a 1-D walk — one needs a genuinely higher-dimensional walk
(lattice-shaped, as in G-R's construction), whose geometry reintroduces boundary losses (G-R's own
caveat: walks leave the search space; fruitless cycles must be filtered).

This analysis bears on a recent **unpublished preprint** (referred to here as the "Churchwell
preprint"; see §8.3) claiming a 1-D walk construction with constant √(π/2) ≈ 1.2533 by merging
endomorphism-accelerated channels into a single walk. We were unable to verify the claim: the
preprint's reference implementation does not build or run as released, and its merge step — the
point where multiple class channels are folded into one displacement ledger — does not conserve
the exponent algebra (a collision merged across channels is attributed to a target offset that the
channel data do not determine). We report this as our assessment of a non-verifiable artifact, not
as a refereed finding; if a buildable construction appears, §6.2's obstruction says what it must
overcome.

### 6.3 The √(π/2) ≈ 1.2533 "floor" and why SOTA+ sitting below it is not a contradiction

The number 1.2533 is recognizable: √(π/2) = 1.25331…, the mean of the Kolmogorov distribution
E[sup_t |B_t|] for standard Brownian motion on [0,1] — the constant that surfaces when a
diffusion-style meeting-time argument for walk-based birthday structures is made precise. It
circulates in the community literature (and in the unpublished preprint of §6.2/§8.3) as a floor
for 1-D kangaroo walks.

Taken literally as a floor over all 1-D walks, the figure is already falsified by measurement:
plain SOTA — one new class-sample per operation — measures K = 1.15 at the Kang-1 configuration
and 1.15 ideal across our whole range sweep. The figure can therefore only be a statement about a
restricted family (most plausibly: walks with the *standard* full-interval herd geometry and a
diffusive sign process, i.e. the classic lineage where 2.0 → 1.715 → 1.661 live — our measured
constants for that lineage, 2.11/1.61/1.35 at the Kang-1 configuration, all sit above 1.2533).
Since the preprint is unverifiable (§8.3) we cannot check its hypotheses; we flag the tension
rather than paper over it.

For SOTA+ the arithmetic works out cleanly, which is presumably why the two-sample reading
circulates: each SOTA+ operation offers *two* class-samples (the main candidate and the
alternative candidate share one inversion), and any per-operation birthday floor scales as
1/√(samples per operation): 1.2533/√2 ≈ **0.886**. Our SOTA+ v2 ideal-equivalent (1.004–1.009)
sits between the two-sample floor 0.886 and the single-sample floor 1.2533 — below the latter,
comfortably above the former. So: no contradiction with the two-sample reading; a measured
contradiction with the unrestricted single-sample reading; and an obstruction (§6.2) saying that
any construction which approaches these floors must leave the 1-D endomorphism-class template.

---

## 7 Discussion

### 7.1 What the 1.15 is and is not

K = 1.15 is a property of four things at once: the adaptive-sign walk (confined, §3.2), the herd
geometry (tames in W/32, wilds over 1.0625W, pre-positioned), the two-channel collision algebra,
and a chain-preserving loop handler. Remove any one and the constant degrades measurably:
naive loop handling → 1.95 at r = 47 (§5.4); wrong herd scale → +7–14% (§5.5); directed walk
(classic) → 2.1–2.4; three-herd without the symmetry algebra (3-way) → 1.6–1.9. The constant is
*not* a property of the walk alone, and we found no evidence it can be improved by jump-table
tuning (§5.7: ±1.5% over a 4× jump-scale range) — the remaining gap to the two-sample floor
(0.886) is, on our evidence, structural rather than tunable.

### 7.2 The honest accounting of SOTA+

SOTA+ beats plain SOTA only if the second point costs less than:

| reference config | plain SOTA K | SOTA+ v2 K (second point unpriced) | break-even price per step |
|---|---|---|---|
| n = 41, dp 5 | 1.1537 ± 0.0195 | 1.0093 ± 0.0170 | **0.143 op** |
| n = 32, dp 5 | 1.4012 ± 0.0091 | 1.1522 ± 0.0079 | 0.216 op |
| n = 48, dp 14 (esc) | 1.7601 ± 0.1045 | 1.5090 ± 0.0963 | 0.166 op |

(per-computed-alternative break-evens: v0, which computes the alternative on half the steps:
0.253 op; v1, a quarter: 0.375 op.) What does the second point actually cost?

* *Unbatched inversion* (a straightforward CPU implementation: one modular inversion per
  addition — tens to hundreds of mul-equivalents, depending on the inversion algorithm): the
  alternative shares the inversion; its marginal cost is one λ-multiplication plus one squaring,
  ≈ 0.01–0.05 op. **SOTA+ is profitable here** — consistent
  with RC's framing ("if inversion calculation takes a lot of time, this second point is cheap for
  us") and with Kang-1 being a CPU demonstrator.
* *Batched inversion* (GPU, RC's triple-Montgomery trick amortizing inversion to ~3% of a step): a
  step is ≈ 2 mul + 1 sqr (+3% inversion); the alternative adds ≈ 1 mul + 1 sqr (affine, reusing
  the shared inverse) — possibly less with an x-only differential-addition formulation, which we
  have not verified for short-Weierstrass curves — i.e. ≈ 0.3–0.7 op,
  **well above every break-even above**. SOTA+ is unprofitable on batched-inversion GPUs.
* Consistently, **RC's own GPU implementation ships plain SOTA**: we verified in
  `RCGpuCore.cu` that the kernel computes a single candidate per step (sign from y-parity, index
  from x), performs L1S2 loop detection and two-table escape, and contains no branch-tag
  selection. The GPU-relevant constant of the SOTA family is 1.15 (plus the DP-overhead formula),
  not 0.99. RC's README sentence "pay 1MUL+1SQR and get K about 0.99 (preferable for GPU
  implementation)" is, under our accounting, correct about the constant and incorrect about the
  economics; we record the discrepancy rather than resolve it (the second point's y-handling in
  Kang-1's unoptimized form admits several priced readings, and the author's own GPU artifact
  declined the option).

Two second-order effects count against SOTA+ in practice: its elevated DP rate (1/18.3 vs 1/32 at
dp 5) grows the DB 1.75× per operation, and its stronger dependence on low-bit oracle structure
sharpens the adversarial caveat of §4.4.6.

### 7.3 Implications for implementations

1. **Loop handling is not optional at scale.** Any implementation targeting r ≥ 48 must implement
   a chain-preserving escape (Kang-2's two-table method); naive restarts cost 10–25% (and 79% at
   dp 20) and can silently *fail* at extreme dp (§5.5 failure mode).
2. **Herd size and DP bits must co-scale with the range.** The spacing/band ratio is the governing
   geometric quantity; a practical rule of thumb from our data is wild spacing ≲ band width
   (spacing/band ≲ 1), i.e. h ≳ 3·1.0625·W/(2·B) with B ≈ 400·√W·(M/2^14)^0.4 — and the DP bits
   then follow from the DPK target of the RC formula. Under-tiling costs +7–14%; over-tiling
   wastes hardware (the formula's overhead term starts to over-penalize, −9…−22%).
3. **The DP formula is a good planner.** Within its domain (§5.3) it tracked every configuration
   we ran to ≤ 2.5%; use it, but re-derive DPK with the *effective* DP rate when running SOTA+
   selection rules (their dp rate is not 1/2^d — §3.3's table).
4. **Tag sourcing in SOTA+.** Substituting an independent tag source for the shared low-bit tag
   keeps the concentration half of the gain but forfeits the DP-rate elevation — worth ≈ 0.004 of
   K at the Kang-1 configuration (statistically zero there) but up to ≈ 0.11 at 2^31/dp5 and
   growing with the DP-overhead share (§3.4). If DP density matters to you (DB size), keep the
   shared tag.
5. **The mirror idea is dead** (§6.1) — as a K optimization. As a DB-size/K trade it is dominated
   by simply lowering dp.

### 7.4 Open problems

(a) A first-principles derivation of K = 1.15 from the band geometry and the two-channel algebra —
our confinement measurements supply the missing geometric ingredient (band width, saturation time,
revisit rate) that any such derivation must consume. (b) The exponent in Law 3.2: the control
experiment (§3.4) shows the law survives re-keying the tag, so the concentration is over
position-derived classes, not DP classes specifically; a Markov-chain analysis of the DP-seeking
walk (the walk is *captured* by adjacent DPs rather than landing on them) is the natural next
step, as is an explanation of the real-curve SOTA+ point sitting 2.1σ below the model (§5.6).
(c) The n = 48 low-DPK residuals (+10–15% at dp ≤ 14, h = 512) — we attributed
them to herd geometry by monotonicity but did not model them. (d) Whether the SOTA family's
algebra admits a *third* cheap sample (e.g. storing both P + J and P − J when both hit a DP) — our
mirror experiment warns that added samples help only if their candidates stay in-window.

---

## 8 Ethics and attribution

### 8.1 Priority

The SOTA and SOTA+ methods, the loop-handling construction, the DP-overhead formula, and the
published constants 1.15/1.02/1.05/0.99 are the work of **RetiredCoder** (RC), released 2024–2025
as source (Kang-1, Kang-2, RCKangaroo, GPLv3) and discussed publicly on the Bitcointalk research
thread [RC, thread]. RC states he "couldn't find any papers about this method". This paper makes
no claim on the method itself; its contributions are independent measurement, mechanism analysis,
validity delimitation, and negative results. We have deliberately structured §3 so that a reader
can implement the family from our specification alone, but the specification is a reading of RC's
published code, and priority is his.

### 8.2 Community verification

The constants have been community-verified by independent runs of RC's own programs on the
Bitcointalk thread; what was missing — and what this paper adds — is (i) an independent
reimplementation of the *walk statistics* (not the code) with controlled oracles, (ii) statistical
uncertainty quantification, (iii) mechanism analysis and falsification tests, and (iv) validity
boundaries for the published formulae.

### 8.3 On the "Churchwell" preprint

We reference an unpublished preprint claiming a 1-D constant of √(π/2) ≈ 1.2533. We could not
locate a buildable implementation or a verifiable version of the argument, and our §6.2 analysis
identifies a specific obstruction any such construction must address; our critique (unbuildable
code, unsound merge argument) is an assessment of an artifact we could not verify and should be
read as inviting correction rather than as a settled judgment. We chose to report this rather than
suppress it because the 1.2533 figure circulates in the community as an established floor; if the
preprint's author makes a verifiable version available, we will gladly re-examine. Until then, the
floor rests on the kind of evidence most community folklore rests on: it was too good not to
repeat, and nobody had the code to check.

### 8.4 Dual-use note

Interval ECDLP for secp256k1 sits at the intersection of a public puzzle economy (the Bitcoin
puzzles, whose bounties motivate most of this engineering), security research, and theft against
holders of weak-key funds. The methods analyzed here are public; this paper publishes measurements
and analysis, not new attack capability. All artifacts analyzed are third-party open-source code.
The quantitative results here (constants, validity domains, failure modes) are equally usable to
*defend* — e.g., to size the cost of key-recovery from exposed low-entropy keys, which is the
standard argument for rejecting low-entropy nonces.

---

## 9 Reproducibility

All instruments are single-file C programs (Appendix A lists paths, builds and the exact run
matrix); every number in this paper is a line of one of the cited output files, and the artifacts
we re-ran to verify file integrity (the 1v1 sweep and the EC cross-checks) reproduced the archived
summaries exactly. We mark every number that is a model estimate rather than a measurement and
every censored sample. The paper should not be submitted or cited before the
two flagged open items (the real-curve SOTA+ discrepancy of §5.6, and a
higher-trial replication of the independent-tag control) are resolved.

---

## Acknowledgements

The author thanks RetiredCoder for publishing the SOTA family in source form and answering
community questions on the research thread; the Bitcointalk puzzle community for a decade of
empirical ground truth; and the maintainers of the OpenSSL project, whose arithmetic backs the
real-curve cross-checks. RC introduced SOTAv2 with "I couldn't find any papers about this method,
so let's assume that I invented it :)" [RC, Kang-1]. Both halves check out: the priority is his,
and this paper exists to confirm the constants he quoted next to the smiley. Simulation and
analysis tooling was developed with the assistance of Claude Code (Anthropic); all conclusions,
errors and interpretations are the author's.

## References

1. J. M. Pollard, "Monte Carlo methods for index computation (mod p)", *Mathematics of
   Computation* 32 (1978), 918–924.
2. J. M. Pollard, "Kangaroos, monkeys and Kruskal's algorithm", unpublished manuscript (2000).
3. P. C. van Oorschot, M. J. Wiener, "Parallel collision search with cryptanalytic applications",
   *Journal of Cryptology* 12 (1999), 1–28.
4. S. D. Galbraith, J. M. Pollard, R. S. Ruprai, "Computing discrete logarithms in an interval",
   IACR ePrint 2010/617 (published in *Mathematics of Computation*); constants 1.715 and 1.661.
5. S. D. Galbraith, R. S. Ruprai, "Using equivalence classes to accelerate solving the discrete
   logarithm problem in a short interval", IACR ePrint 2010/615; PKC 2010, LNCS 6056; constant
   ≈ 1.36 and the practical caveats quoted in §2.1.
6. F. Gaudry, E. Schost, "A low-memory algorithm for solving discrete logarithm problems in short
   intervals" (for the Gaudry–Schost-style variant referenced by [4]).
7. RetiredCoder, "Kang-1: ECDLP kangaroo methods demonstrator (Classic, 3-Way, Mirror, SOTA,
   SOTA+)", GitHub, 2024. Includes `ExpKangDlg.cpp` (source of the SOTA/SOTA+ semantics quoted in
   §3.1) and `README.md` (the published constants).
8. RetiredCoder, "Kang-2: loop handling for the SOTA method", GitHub, 2024–2025. `README.md`
   (simple vs advanced loop handling, large-loop statistics) and `ExpKangDlg.cpp` (two-table
   escape, L2 loops).
9. RetiredCoder, "RCKangaroo: GPU (CUDA) implementation of SOTA v2 for secp256k1", GitHub,
   2024–2025, v4.0. `README.md` (K = 1.15, throughput, DP formula), `RCKangaroo.cpp` line 351
   (the DP-overhead formula), `RCGpuCore.cu` (single-candidate walk kernel — the basis of §7.2's
   GPU conclusion).
10. RetiredCoder, Bitcointalk research thread 5517607 ("ECDLP kangaroo methods"), community
    verification record.
11. "Churchwell" (pseudonymous?), unpublished preprint on 1-D endomorphism-accelerated kangaroo
    walks and the √(π/2) ≈ 1.2533 constant. **Unverifiable at the time of writing; see §6.3, §8.3.**
12. Bitcoin puzzle transaction community documentation (the 2^n-bounty puzzle series), for the
    motivation and the r ∈ [32, 160] practical range regime.

*Institutional-constants note:* √(π/2) = 1.253314…; the Kolmogorov distribution's mean E[sup_t |B_t|]
for standard Brownian motion on [0,1] equals √(π/2); we cite the constant's arithmetic identity,
not a priority claim on its use.

## Appendix A. Artifact manifest and run matrix

All paths are from the analysis host; the raw outputs are quoted verbatim in the tables above.

**Instruments (this work):**

| file | role |
|---|---|
| `kangsim_final.c` | primary Z/p hash-oracle simulator; methods 0 (1v1 ideal), 1 (classic herd), 2 (3-way), 6 (mirror), 3 (SOTA), 4 (SOTA+ v0/v1/v2); flags `-n -t -k -d -M -v -w -mirror -esc -jm -js -s`; built `clang -O3 -march=native` |
| `kangsim6.c` | as above + `-it` (independent-hash branch tag) — the §3.4 control |
| `eckang.c` | real secp256k1 ground truth (OpenSSL); SOTA/SOTA+ with shared inversion; `-R` matches `-n−1` of kangsim |
| `walktest.c` | single-walk displacement statistics, EC vs Z/p |
| `hashtest.c` | oracle DP-rate uniformity |

**Third-party sources analyzed (not modified):**

| path | content |
|---|---|
| `Kang-1/` | RC part 1: `README.md` (published constants), `ExpKangDlg.cpp` (SOTA/SOTA+ semantics, loop handling) |
| `Kang-2/` | RC part 2: `README.md` (loop handling, large-loop statistics), `ExpKangDlg.cpp` (two-table escape) |
| `RCKangaroo/` | RC part 3: `README.md`, `RCKangaroo.cpp` (DP formula, line 351), `RCGpuCore.cu` (walk kernel) |

**Run matrix (output files in `out/`, all numbers above traced to them):**

* A-batch, n = 41, 4000 trials (Kang-1 exact): `A_classic`, `A_3way`, `A_sota`, `A_sotap_v0/v1/v2`,
  `A_sotap_v0_paid`, `A_sotap_v2_paid`, `A_sota_mirror(+_paid2)`, `A_sotap_mirror(+_paid)`,
  `A_mirror`, `A_1v1_jm{0.5,1,3}` (1500/800/1500 trials).
* B-batch, n = 32, 20 000 trials (12 000 for sweeps): `B_classic`, `B_3way`, `B_mirror`,
  `B_sota`, `B_sotap_v0/v1/v2`, `B_sota_mirror`, `B_sotap_mirror`, dp sweep `B_sota_dp{4,6,8,10,12,14}`,
  herd sweep `B_sota_k{64,128,2048,8192}`, table sweep `B_sota_M{9,11}`, jump sweep `B_sota_js{2,4}`,
  `B_1v1_jm{0.5,1,2,3,4}_clean` (re-run for this paper; the archived copies of the original five
  files contain interleaved output from two concurrent runs and were re-verified against the
  clean reruns, which reproduce their summary lines exactly).
* C-batch, n = 48, 100–300 trials: `C_classic`, `C_3way`, `C_mirror`, `C_sota`, `C_sotap_v2`,
  dp sweep `C_sota_dp{12,14,16,18,20,24}`, escape variants `C_sota_dp{12,14,16,18}_esc`,
  `C_sotap_dp14_esc`, `C_sota_dp14_js4_esc`, `C_sota_js{4,5}`, herd sweep `C_sota_k{2048,8192}`,
  `C_sota_k8192_dp12_esc`, `C_sota_k2048_dp14_esc`, `C_sota_k32k_dp16_esc`, `C_sota_M9`,
  `C_sota_mirror`.
* EC cross-checks: `EC_sota*`, `EC_sotap*` (r = 32) and paired r = 31 runs (`EC_sota_R31_dp5`,
  150 trials; `EC_sotap_R31_dp5`, 100 trials; `EC_sota_R31_dp12`, 30 trials) — §4.2.2.
* Independent-tag controls (`kangsim6.c -it`): `IT_v2_n32` (12 000 trials, K = 1.2653 ± 0.0105),
  `IT_v0_n32` (12 000, K = 1.2626 ± 0.0104), `IT_v2_n41` (4 000, K = 1.0126 ± 0.0166), plus the
  200-trial pilot (`IT_v2_n32_pilot`) quoted in §3.4's methodological warning, and `bandm_results.txt`
  (the §3.2 confinement sweep, from `bandm.c`).
* Band measurements (§3.2): `bandm.c` sweep, 40 trials × {10^3, 10^4, 10^5, 10^6} steps ×
  M ∈ {2^6,…,2^16}.

**Build/run examples.**

```
clang -O3 -march=native kangsim_final.c -o kangsim -lm
./kangsim -m 3 -n 41 -t 4000 -k 512 -d 5            # SOTA at the Kang-1 configuration
./kangsim -m 4 -v 2 -n 41 -t 4000 -k 512 -d 5       # SOTA+ v2
./kangsim -m 3 -n 48 -t 200 -k 512 -d 14 -esc       # SOTA with escape loop handling
clang -O2 eckang.c -o eckang -I<openssl>/include -L<openssl>/lib -lcrypto
./eckang -m 3 -R 31 -t 150 -d 5                     # real-curve SOTA, paired geometry
```

**Determinism.** Each run fixes one seed (default `0xABCDEF0123456789`, `-s` to override); jump
tables are drawn once per run; per-trial randomness streams from the same generator. Re-running
the same command reproduces the archived file byte-for-byte (verified for the 1v1 sweep and
spot-checked elsewhere); cross-seed variance of K is bounded by the jump-table fluctuation
estimate of §4.4.2.
