# Kangaroo walk constants for interval ECDLP on secp256k1 — empirical validation

**Author:** Decruptor (pseudonymous)
**Status:** preprint draft, v0.1. Paper PDF/text: [`paper.md`](paper.md) (one-page version in [`abstract.md`](abstract.md)).
**ePrint:** [eprint-url] *(to be filled in on submission)*

## What this is

An independent, large-sample measurement of the walk constants of the "SOTA" and "SOTA+"
kangaroo methods for the interval discrete-logarithm problem on secp256k1 — an unpublished
method family by RetiredCoder (RC) that underlies RCKangaroo, the fastest publicly known
solver for the Bitcoin-puzzle ranges. Using a hash-oracle model of the group, we ran
~5.1·10^5 full DLP solves across 70 configurations at interval widths 2^31–2^47, and
cross-validated against real secp256k1 runs (OpenSSL) at matched geometry.

## Plain-language summary

In 2024–2025 the pseudonymous author RetiredCoder published, as source code with a minimal
README, a kangaroo method family with a claimed cost constant K = 1.15 (and 0.99–1.05 for a
"cheap second point" variant) — far below anything in the published literature (no published
method is below ≈ 1.36 for this problem). Because these constants directly set the economics
of every interval-ECDLP effort on secp256k1, we rebuilt the walk from RC's code, simulated it
under a controlled hash oracle, and measured. Every published constant reproduces within
±0.02. The DP-overhead formula RC ships in RCKangaroo holds to ≤ 2.5% across four orders of
magnitude of its load parameter, and we chart where it breaks (small herds at wide intervals,
sparse-DP pathology, very large herds). Mechanistically: the SOTA walk is *confined* — it
saturates in a band of ≈ 350–800·√W instead of diffusing — which explains the tiny start
ranges, the loop-handling fragility (naive restarts land at K = 1.95 at 2^47), and why the
0.99 "GPU" constant of SOTA+ is really a CPU number: RC's own GPU kernel ships plain SOTA.
Negative results: the "mirror" second sample is unprofitable at any price, and the
√(π/2) ≈ 1.2533 "floor" for 1-D walks is falsified as an unrestricted claim.

## Headline numbers

At the exact Kang-1 configuration (2^40, 512 kangaroos, DP bits 5, jump table 2^14, 4 000 trials):

| method | RC published | this work |
|---|---|---|
| classic (herd) | ≈ 2.1 | 2.1113 ± 0.0348 |
| 3-way | ≈ 1.6 | 1.6141 ± 0.0284 |
| SOTA | 1.15 | 1.1537 ± 0.0195 |
| SOTA+ v0 (second point free) | 1.02 | 1.0242 ± 0.0171 |
| SOTA+ v1 (pay (1MUL+1SQR)/4) | 1.05 | 1.0548 ± 0.0180 |
| SOTA+ v2 (pay 1MUL+1SQR; "GPU") | 0.99 | 1.0093 ± 0.0170 |

All agree within ±0.02. 20 000-trial runs at 2^31, dp/herd/jump sweeps, loop-handling and
herd-scaling fragility measurements, the SOTA+ selection-rule model, and the negative results
are in the paper.

## Context

The motivating application is the Bitcoin puzzle transaction (funded 2015-01-15, 256 outputs of
n × 100,000 sats; pubkeys for every 5th puzzle #65–#160 revealed via dust in 2019, which is
what makes them kangaroo-attackable). As of October 2026, 83 of 256 puzzles are claimed; the
only remaining pubkey-known targets are #140, #145, #150, #155 and #160 (~75 BTC total), ranging
from tens of millions to tens of billions of GPU-hours away at current constants — which is
precisely why the constant in front of √W matters.

## Repository layout

```
paper.md          the preprint (markdown, self-contained)
abstract.md       one-page standalone abstract
simulators/       the measurement instruments (single-file C, see below)
```

## Build and run

All instruments are single-file C programs. Any C99 compiler works; only `eckang.c` needs
OpenSSL (libcrypto).

```sh
# Primary simulator: hash-oracle model of the group (Z/p), all methods
clang -O3 -march=native kangsim_final.c -o kangsim -lm
./kangsim -m 3 -n 41 -t 4000 -k 512 -d 5            # SOTA at the exact Kang-1 configuration
./kangsim -m 4 -v 2 -n 41 -t 4000 -k 512 -d 5       # SOTA+ v2
./kangsim -m 3 -n 48 -t 200 -k 512 -d 14 -esc       # SOTA with escape loop handling

# Control variant with the independent-hash branch tag (paper §3.4)
clang -O3 -march=native kangsim6.c -o kangsim6 -lm
./kangsim6 -m 4 -v 2 -n 41 -t 4000 -k 512 -d 5 -it  # SOTA+ v2, independent tag

# Real secp256k1 cross-check (OpenSSL), matched geometry
clang -O2 eckang.c -o eckang -I<openssl>/include -L<openssl>/lib -lcrypto
./eckang -m 3 -R 31 -t 150 -d 5                     # real-curve SOTA, r = 31

# Single-walk displacement statistics, EC vs model (paper §4.2.1) — needs OpenSSL
clang -O3 -march=native walktest.c -o walktest -I<openssl>/include -L<openssl>/lib -lcrypto
./walktest

# Oracle DP-rate uniformity (paper §4.2.3)
clang -O3 -march=native hashtest.c -o hashtest
./hashtest
```

Flags (kangsim): `-m` method (0 = 1v1 ideal, 1 = classic herd, 2 = 3-way, 3 = SOTA, 4 = SOTA+,
6 = mirror), `-v` SOTA+ variant (0/1/2), `-n` log2 interval width, `-t` trials, `-k` herd size,
`-d` DP bits, `-M` log2 jump-table size, `-w` price the second point, `-mirror` mirror trail,
`-esc` escape loop handling, `-jm`/`-js` jump-mean/jump-scale sweeps, `-s` seed. Each run fixes
one seed (default `0xABCDEF0123456789`) and draws fresh jump tables per run; re-running the same
command reproduces the archived output byte-for-byte.

Note: the sources in `simulators/` were copied from the analysis snapshot (`/tmp/deepdive-sim`,
2026-10-23). The raw per-configuration outputs (`out/A_*`, `out/B_*`, `out/C_*`, `out/EC_*`,
independent-tag controls, band sweep) are the paper's Appendix A run matrix; if you want them
in the repo, copy the `out/` directory from the snapshot next to the sources.

## Provenance and attribution

The SOTA and SOTA+ methods, the loop-handling construction, the DP-overhead formula and the
published constants are RetiredCoder's work (Kang-1, Kang-2, RCKangaroo, GPLv3, 2024–2025),
discussed publicly on Bitcointalk thread 5517607. This repository contains measurements and
analysis of that published code, not a new method; priority is RC's. Simulation and analysis
tooling was developed with the assistance of Claude Code (Anthropic); all conclusions, errors
and interpretations are the author's.

## License

GPLv3 — see [`LICENSE`](LICENSE). Copyright (C) 2026 Decruptor.

## Citing

See [`CITATION.cff`](CITATION.cff). If the ePrint number exists, cite the ePrint version;
this repository is the code-and-data companion.
