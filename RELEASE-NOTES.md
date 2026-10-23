# Release notes

## v0.1 — initial release (DRAFT, not yet published)

Date: 2026-10-23 (draft prepared; release date = actual publication date)

Initial public release of the kangaroo-constants paper and its simulators.

Contents:

- `paper.md` / `abstract.md` — the preprint: independent measurement of the
  SOTA / SOTA+ walk constants (RetiredCoder's unpublished kangaroo family),
  ~5.1e5 full DLP solves across 70 configurations at 2^31–2^47, validation and
  delimitation of the RC DP-overhead formula, mechanism analysis (walk
  confinement, SOTA+ selection-rule model, decomposition control), fragility
  quantification (loop handling, herd scaling), and negative results (mirror
  second sample, sqrt(pi/2) floor).
- `simulators/` — single-file C instruments: `kangsim_final.c` (primary
  hash-oracle simulator), `kangsim6.c` (independent-tag control), `eckang.c`
  (real secp256k1 cross-check, OpenSSL), `walktest.c`, `hashtest.c`.
- `LICENSE` — GPLv3.

Known gaps at release time:

- ePrint number: placeholder `[eprint-url]` in README.md and CITATION.cff —
  fill in after ePrint acceptance.
- The raw per-configuration outputs of the Appendix A run matrix (`out/`
  batch files) are not yet in the repo; copy them from the analysis snapshot
  if a data release is wanted (recommended before posting the repo link
  publicly).
- Two flagged open items in the paper (§9) should be resolved or annotated
  before or at submission: the real-curve SOTA+ 2.1σ discrepancy (§5.6) and a
  higher-trial replication of the independent-tag control (§3.4).

Placeholders to replace before publishing: `[eprint-url]`, `[repo-url]`.
