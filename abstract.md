# An empirical analysis of Pollard-kangaroo walk constants for interval ECDLP on secp256k1, including the unpublished SOTA family

*Full abstract — draft for review, not submitted. The ~250-word trim for the IACR ePrint
submission form follows at the bottom of this file.*

**Background.** For the elliptic-curve discrete logarithm problem in an interval of width W, the
kangaroo method's cost is K·√W group operations, and the constant K is the entire practical
difference between methods. The published lineage runs K ≈ 2 (two herds), 1.715 (four herds,
Galbraith–Pollard–Ruprai 2010), 1.661 (Gaudry–Schost variant), ≈ 1.36 (equivalence classes,
Galbraith–Ruprai 2010). Below that sit the *unpublished* SOTA and SOTA+ methods of RetiredCoder
(RC) — the walk family behind the fastest public secp256k1 interval-ECDLP solver (RCKangaroo,
~14.5 GH/s on an RTX 4090) — with claimed constants K = 1.15 and, using a "cheap second point",
1.02/1.05/0.99. Until now these numbers had no independent, statistically qualified verification
and no mechanistic explanation.

**Method.** We built a hash-oracle model of the group (prime-field additive group; the EC
x-coordinate's oracles — jump index, DP test, SOTA+ branch tag — modeled as shared low bits of one
hash of the ±-invariant canonical form; the y-parity sign bit modeled as an antisymmetric hash
bit), and ran ~5.1·10^5 full DLP solves in 70 configurations at interval widths 2^31–2^47,
cross-validated against real secp256k1 runs (OpenSSL) at matched geometry and by single-walk
displacement statistics.

**Results.**
1. All of RC's published constants reproduce within ±0.02: SOTA 1.1537 ± 0.0195, SOTA+ 1.0242 /
   1.0548 / 1.0093, classic 2.1113, 3-way 1.6141, at the exact Kang-1 configuration (2^40, 512
   kangaroos, DP bits 5, 4 000 trials; 20 000 trials at 2^31).
2. RC's DP-overhead formula K = 1.15 + (0.07 + 0.76/√DPK)/(1 + 0.30·DPK) is validated to ≤ 2.5%
   over DPK ∈ [0.025, 74] at the calibration herd and range, and its three failure regimes are
   charted: small herds at wide intervals (+7–14%, geometric), sparse-DP pathology with naive loop
   handling (+15% to +79%; at DP bits 24 and 512 kangaroos at 2^47, 48 of 60 trials fail outright),
   and very large herds (−21.5% at 32 768 kangaroos).
3. Mechanism: the adaptive-sign walk is *confined* — displacement saturates at a band of width
   ≈ 350–800·√W within ~10^3–10^4 steps instead of diffusing — which explains the tiny start
   ranges, the loop-handling fragility, and the herd-size law (the DP formula holds while wild
   start positions tile the interval at least as densely as the band). For the SOTA+ rules we
   derive the selection-induced DP rate *exactly* (1/32, 1/21.33, 1/25.6, 1/18.29 predicted;
   measured 1/32.0, 1/21.3, 1/25.6, 1/18.3) and an empirical Herfindahl scaling
   K/K_SOTA ≈ √(H_unif/H_sel) reproducing every SOTA+ measurement to ≤ 1%.
4. A control experiment re-keying the branch tag to an independent hash collapses the DP rate to
   exactly 1/32 and removes precisely the DP-overhead share of the gain while the concentration
   share survives — decomposing the SOTA+ advantage into two individually validated effects, and
   overturning (at proper sample size) the pilot-scale impression that the gain requires bit-sharing.
5. Negative results: a correlated "mirror" second sample gains ≤ 3.6% ± 1.3% at zero price and
   loses money at any real price (its collision candidates land ~W/32…W outside the window); a 1-D
   endomorphism-accelerated walk is obstructed on secp256k1 because GLV λ-orbits preserve no
   interval of width W ≪ n — which bears on an unverifiable unpublished preprint claiming a
   √(π/2) ≈ 1.2533 floor, a figure already falsified as an unrestricted claim by the measured
   1.15, while SOTA+'s 1.00–1.02 sits between the two-sample floor 1.2533/√2 ≈ 0.886 and it.
6. Honest accounting: SOTA+ beats plain SOTA only if the second point costs < 0.14–0.22 op/step.
   That holds under unbatched inversion (extra ≈ 0.01–0.05 op) and fails under GPU batched
   inversion (extra ≈ 0.3–0.7 op); consistently, RC's own GPU kernel ships plain SOTA — verified
   in source — so the GPU-relevant constant is 1.15, not 0.99.

**Attribution and ethics.** The SOTA family, its loop handling, its formulae and its constants are
RetiredCoder's work, published as source and community-verified on the Bitcointalk research
thread; this paper contributes independent measurement, mechanism analysis, validity boundaries,
and negative results, and claims no priority on the method. The puzzles motivate the work; the
same quantities size the cost of recovering low-entropy keys, which is the standard argument for
rejecting weak nonces. Full data, simulator sources and the run matrix are reproducible from
single-file C programs listed in the paper's appendix.

---

## ePrint submission trim (~250 words)

To our knowledge, we report the first independent, large-sample validation of the walk constants of
the "SOTA" and "SOTA+" kangaroo methods for interval ECDLP on secp256k1 — an unpublished family due
to RetiredCoder that underlies the fastest publicly known solvers for the Bitcoin-puzzle ranges.
Using a hash-oracle model of the group, we run ~5.1·10^5 full DLP solves across 70 configurations
at interval widths 2^31–2^47, reproducing all published constants: SOTA K = 1.15 (measured
1.1537 ± 0.0195), SOTA+ 1.02/1.05/0.99 (measured 1.0242, 1.0548, 1.0093), classic ≈ 2.1, 3-way
≈ 1.6. RC's distinguished-point overhead formula is validated to ≤ 2.5% over DPK ∈ [0.025, 74] at
the calibration herd and range, and its failure regimes are charted quantitatively. The SOTA walk
is confined: displacement saturates at ≈ 350–800·√W within 10^3–10^4 steps, explaining tiny start
ranges and why loop handling dominates at large ranges (naive restarts degrade 1.15 → 1.95 at
2^47). For the SOTA+ rules we derive the per-step DP rate exactly (1/32, 1/21.33, 1/25.6, 1/18.29)
and an empirical Herfindahl law reproducing every constant to ≤ 1%. Re-keying the branch tag to an
independent hash decomposes the SOTA+ gain into a concentration component (10.0% of the 17.8%
total gain at 2^31/dp5) and a DP-synergy component worth exactly the DP-overhead share. The 0.99
"GPU" constant is a CPU number: the second point must cost < 0.14–0.22 op/step — true for
unbatched CPU inversion (≈ 0.01–0.05 op), false on batched-inversion GPUs; RC's own GPU kernel
ships plain SOTA. Negative results: the correlated mirror sample is unprofitable at any price;
1-D endomorphism-accelerated walks are obstructed because GLV λ-orbits preserve no interval of
width W ≪ n, falsifying the claimed √(π/2) ≈ 1.2533 floor as an unrestricted statement.
