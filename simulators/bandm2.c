// bandm2.c — rebuild of the paper §3.2 band instrument, with cycle detection.
//
// The SOTA walk in the Z/p hash-oracle model is a DETERMINISTIC map on the state x:
// next(x) = x +- jmpdist[idx(x)], sign from x. On a finite state space every trajectory
// is eventually periodic. This instrument measures, per raw walk:
//   mu   = tail length (steps before the state repeats),
//   lam  = cycle period,
//   band = (max dist - min dist) over the full trajectory (mu + lam steps) plus one
//          extra period, confirming the band stops growing at cycle entry,
// plus the band the original step-budget cutoffs (1e3/1e4/1e5/1e6) would report.
// Oracles, jump table and start rules are copied verbatim from kangsim_final.c
// (mix64 oracle of canon(x), adaptive sign, SOTA starts: tame d in [0,W/32),
// wild even d in [0,34*W/32) at -k', k' = k + W/32).
//
// Build: clang -O3 -march=native bandm2.c -o bandm2 -lm
// Run:   ./bandm2 -n 32 -M 14 -w 40
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

typedef uint64_t u64; typedef int64_t s64; typedef uint32_t u32;

#define PMOD 2305843009213693951ULL /* 2^61-1 */

static inline u64 mix64(u64 x) {
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}
static u64 g_rng;
static inline u64 rnd64(void) { g_rng += 0x9E3779B97F4A7C15ULL; return mix64(g_rng); }
static inline u64 rndmax(u64 r) { return rnd64() % r; }
static inline u64 canon(u64 x) { u64 y = PMOD - x; return x < y ? x : y; }
static inline u64 padd(u64 a, u64 b) { u64 s = a + b; if (s >= PMOD) s -= PMOD; return s; }
static inline u64 psub(u64 a, u64 b) { return a >= b ? a - b : a + (PMOD - b); }
static inline u64 modn(s64 v) { s64 m = v % (s64)PMOD; if (m < 0) m += (s64)PMOD; return (u64)m; }

static int RR = 31, MLOG = 14, NWALKS = 40;
static int ESCMODE = 0; static u64 ESCWIN = 48000;
static u64 W, X32, jmpdist[1 << 20], jmpdist2[1 << 20];
static double SQRTW;
#define MMASK ((1u << MLOG) - 1)

static inline u64 el_H(u64 e) { return mix64(canon(e) ^ 0x1111111111111111ULL); }
static inline u64 el_index(u64 e) { return el_H(e) & MMASK; }
static inline int el_signbit(u64 e) {
  u64 c = canon(e);
  return ((mix64(c ^ 0x2222222222222222ULL) & 1) ^ (u64)(e != c)) ? -1 : 1;
}

// deterministic SOTA step: returns new x, updates *dist by the signed jump
static inline u64 sota_next(u64 x, s64 *dist) {
  u64 j = jmpdist[el_index(x)];
  int sg = el_signbit(x);
  if (sg > 0) { x = padd(x, j); *dist += (s64)j; }
  else        { x = psub(x, j); *dist -= (s64)j; }
  return x;
}

static void build_jumptable(void) {
  u64 minjump = 1ULL << (RR / 2 + 3);
  for (u64 i = 0; i <= MMASK; i++) {
    u64 d = minjump + rndmax(minjump);
    jmpdist[i] = (d & ~1ULL) % PMOD;   // SOTA family: even jumps
  }
}

// escape-mode step: identical to kangsim_diag's USE_ESC handling — an L1S2 bounce
// (the jump FROM the new point would exactly undo this one) is diverted through the
// second (large-jump) table instead of being taken.
static u64 sota_next_esc(u64 x, s64 *dist, int *esc) {
  u64 j; int sg;
  u64 cur_i = el_index(x);
  if (*esc) { j = jmpdist2[cur_i]; sg = el_signbit(x); *esc = 0; }
  else { j = jmpdist[cur_i]; sg = el_signbit(x); }
  u64 x1 = (sg > 0) ? padd(x, j) : psub(x, j);
  u64 x2 = (sg > 0) ? psub(x, j) : padd(x, j);
  (void)x2;
  int nxj_would_undo;
  u64 nxj = el_index(x1); int nsg = el_signbit(x1);
  nxj_would_undo = (nxj == cur_i && nsg == -sg);
  if (sg > 0) { x = x1; *dist += (s64)j; } else { x = x1; *dist -= (s64)j; }
  *esc = nxj_would_undo;
  return x;
}

// -esc mode: run escape-rule walks with a finite window; report how many walks
// have NOT cycled within the window (exact revisit detection via a hash set of
// visited states — the state is (x, pending-esc)).
typedef struct { u64 key; u32 gen; } SeenEnt;
static SeenEnt *seen; static u64 seenmask; static u32 seengen;

static int esc_walk(int role, u64 win) {
  u64 k = rndmax(W); u64 kprime = k + X32;
  s64 d0; u64 x0; int esc = 0;
  if (role == 0) { d0 = (s64)rndmax(X32); x0 = modn(d0); }
  else { d0 = (s64)(rndmax(34 * X32) & ~1ULL); x0 = psub(modn(d0), kprime); }
  seengen++;
  s64 dd = d0; u64 xx = x0;
  u64 key0 = (xx << 1) | (u64)(esc != 0);
  u64 h = mix64(key0) & seenmask;
  for (;;) { SeenEnt *e = &seen[h]; if (e->gen != seengen) { e->key = key0; e->gen = seengen; break; }
             if (e->key == key0) return 1; h = (h + 1) & seenmask; }
  for (u64 i = 0; i < win; i++) {
    xx = sota_next_esc(xx, &dd, &esc);
    u64 key = (xx << 1) | (u64)(esc != 0);
    h = mix64(key) & seenmask;
    for (;;) { SeenEnt *e = &seen[h]; if (e->gen != seengen) { e->key = key; e->gen = seengen; break; }
               if (e->key == key) return 1; h = (h + 1) & seenmask; }
  }
  return 0;
}

typedef struct { double tail, period, band, band_growth, maxdisp, b3, b4, b5, b6; int role; } Res;

// one raw walk; Brent cycle detection on the state x
static void walk(int role, Res *r) {
  u64 k = rndmax(W); u64 kprime = k + X32;
  s64 d0; u64 x0;
  if (role == 0) { d0 = (s64)rndmax(X32); x0 = modn(d0); }
  else { d0 = (s64)(rndmax(34 * X32) & ~1ULL); x0 = psub(modn(d0), kprime); }
  s64 junk;
  // --- phase 1: find lam (period) by Brent ---
  u64 tort = x0, hare = sota_next(x0, &junk);
  u64 power = 1, lam = 0;
  while (tort != hare) {
    if (power == lam) { tort = hare; power *= 2; lam = 0; }
    hare = sota_next(hare, &junk);
    lam++;
  }
  // --- phase 2: find mu (tail length) ---
  u64 mu = 0;
  tort = x0; hare = x0;
  for (u64 i = 0; i < lam; i++) hare = sota_next(hare, &junk);
  while (tort != hare) { tort = sota_next(tort, &junk); hare = sota_next(hare, &junk); mu++; }
  // --- phase 3: replay mu+lam steps from the same start, tracking displacement ---
  // (the map is deterministic in x; dist is determined by the jump sequence, so the
  //  replay reproduces the exact trajectory measured during detection)
  s64 dd = d0; u64 xx = x0;
  double mn = 0, mx = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
  u64 total = mu + lam;
  for (u64 i = 0; i < total; i++) {
    xx = sota_next(xx, &dd);
    double dv = (double)(dd - d0);
    if (dv < mn) mn = dv;
    if (dv > mx) mx = dv;
    u64 step = i + 1;
    if (step == 1000) b3 = mx - mn;
    else if (step == 10000) b4 = mx - mn;
    else if (step == 100000) b5 = mx - mn;
    else if (step == 1000000) b6 = mx - mn;
  }
  // a step-budget run that keeps going past cycle entry just retraces the cycle:
  // the band freezes at its cycle-entry value for any larger budget.
  double bandEnd = mx - mn;
  if (total < 1000) b3 = bandEnd;
  if (total < 10000) b4 = bandEnd;
  if (total < 100000) b5 = bandEnd;
  if (total < 1000000) b6 = bandEnd;
  // one extra period: band must not grow further (periodicity check on the band itself)
  double mn2 = mn, mx2 = mx;
  for (u64 i = 0; i < lam; i++) {
    xx = sota_next(xx, &dd);
    double dv = (double)(dd - d0);
    if (dv < mn2) mn2 = dv;
    if (dv > mx2) mx2 = dv;
  }
  r->tail = (double)mu; r->period = (double)lam;
  r->band = mx2 - mn2;                    // the full-trajectory band width
  r->band_growth = (mx2 - mn2) - bandEnd; // growth over one extra period (should be 0)
  r->maxdisp = fabs(mx2) > fabs(mn2) ? fabs(mx2) : fabs(mn2);
  r->b3 = b3; r->b4 = b4; r->b5 = b5; r->b6 = b6; r->role = role;
  (void)k; (void)kprime;
}

static int cmpd(const void *a, const void *b) {
  double x = *(const double*)a, y = *(const double*)b;
  return x < y ? -1 : x > y ? 1 : 0;
}
static double med(const double *v, int n) {
  double *t = malloc(sizeof(double) * n); memcpy(t, v, sizeof(double) * n);
  qsort(t, n, sizeof(double), cmpd);
  double m = (n & 1) ? t[n / 2] : 0.5 * (t[n / 2 - 1] + t[n / 2]);
  free(t); return m;
}

int main(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-n")) RR = atoi(argv[++i]) - 1;
    else if (!strcmp(argv[i], "-M")) MLOG = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-w")) NWALKS = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-esc")) ESCMODE = 1;
    else if (!strcmp(argv[i], "-win")) ESCWIN = strtoull(argv[++i], 0, 10);
    else if (!strcmp(argv[i], "-s")) g_rng = strtoull(argv[++i], 0, 16);
  }
  if (!g_rng) g_rng = 0xABCDEF0123456789ULL;
  W = 1ULL << RR; X32 = 1ULL << (RR - 5); SQRTW = pow(2.0, RR / 2.0);
  printf("# bandm2: n=%d (W=2^%d) M=2^%d walks=%d (alternating tame/wild)%s\n",
         RR + 1, RR, MLOG, NWALKS, ESCMODE ? " [esc mode]" : "");
  if (ESCMODE) {
    build_jumptable();
    // second jump table for the escape rule (large jumps, as kangsim_diag builds it)
    u64 minjump2 = 1ULL << (RR - 10);
    for (u64 i = 0; i <= MMASK; i++) {
      u64 d = minjump2 + rndmax(minjump2);
      jmpdist2[i] = (d & ~1ULL) % PMOD;
    }
    u64 cap = 1024; while (cap < ESCWIN * 2) cap <<= 1;
    seen = malloc(sizeof(SeenEnt) * cap); memset(seen, 0, sizeof(SeenEnt) * cap);
    seenmask = cap - 1;
    u64 cyc = 0, cycT = 0, cycW = 0; int nT = 0, nW = 0;
    for (int i = 0; i < NWALKS; i++) {
      int role = i & 1, c = esc_walk(role, ESCWIN);
      cyc += c;
      if (role == 0) { cycT += c; nT++; } else { cycW += c; nW++; }
    }
    printf("esc-mode window=%llu steps: cycled-within-window=%llu/%d (tame %llu/%d wild %llu/%d)  NOT cycled=%llu (%.1f%%)\n",
           (unsigned long long)ESCWIN, (unsigned long long)cyc, NWALKS,
           (unsigned long long)cycT, nT, (unsigned long long)cycW, nW,
           (unsigned long long)(NWALKS - cyc), 100.0 * (double)(NWALKS - cyc) / NWALKS);
    return 0;
  }
  build_jumptable();
  Res *rs = malloc(sizeof(Res) * NWALKS);
  for (int i = 0; i < NWALKS; i++) walk(i & 1, &rs[i]);
  double *mu = malloc(sizeof(double) * NWALKS), *lam = malloc(sizeof(double) * NWALKS),
         *band = malloc(sizeof(double) * NWALKS), *b3 = malloc(sizeof(double) * NWALKS),
         *b4 = malloc(sizeof(double) * NWALKS), *b5 = malloc(sizeof(double) * NWALKS),
         *b6 = malloc(sizeof(double) * NWALKS), *extra = malloc(sizeof(double) * NWALKS);
  for (int i = 0; i < NWALKS; i++) {
    mu[i] = rs[i].tail; lam[i] = rs[i].period; band[i] = rs[i].band / SQRTW;
    b3[i] = rs[i].b3 / SQRTW; b4[i] = rs[i].b4 / SQRTW; b5[i] = rs[i].b5 / SQRTW;
    b6[i] = rs[i].b6 / SQRTW; extra[i] = rs[i].band_growth;
  }
  double muT = 0, muW = 0, lamT = 0, lamW = 0, bandT = 0, bandW = 0; int nT = 0, nW = 0;
  for (int i = 0; i < NWALKS; i++) {
    if (rs[i].role == 0) { muT += rs[i].tail; lamT += rs[i].period; bandT += rs[i].band / SQRTW; nT++; }
    else { muW += rs[i].tail; lamW += rs[i].period; bandW += rs[i].band / SQRTW; nW++; }
  }
  double muSum = 0, lamSum = 0, bandSum = 0, b3s = 0, b4s = 0, b5s = 0, b6s = 0, exs = 0, exMax = 0;
  double muMin = 1e300, muMax = 0, lamMin = 1e300, lamMax = 0, bandMin = 1e300, bandMax = 0;
  for (int i = 0; i < NWALKS; i++) {
    muSum += mu[i]; lamSum += lam[i]; bandSum += band[i];
    b3s += b3[i]; b4s += b4[i]; b5s += b5[i]; b6s += b6[i];
    exs += extra[i]; if (extra[i] > exMax) exMax = extra[i];
    if (mu[i] < muMin) muMin = mu[i]; if (mu[i] > muMax) muMax = mu[i];
    if (lam[i] < lamMin) lamMin = lam[i]; if (lam[i] > lamMax) lamMax = lam[i];
    if (band[i] < bandMin) bandMin = band[i]; if (band[i] > bandMax) bandMax = band[i];
  }
  printf("tail: mean=%.0f median=%.0f min=%.0f max=%.0f steps  (tame %.0f / wild %.0f)\n",
         muSum / NWALKS, med(mu, NWALKS), muMin, muMax, nT ? muT / nT : 0, nW ? muW / nW : 0);
  printf("period: mean=%.0f median=%.0f min=%.0f max=%.0f steps  (tame %.0f / wild %.0f)\n",
         lamSum / NWALKS, med(lam, NWALKS), lamMin, lamMax, nT ? lamT / nT : 0, nW ? lamW / nW : 0);
  printf("band/√W at cycle entry: mean=%.1f median=%.1f min=%.1f max=%.1f  (tame %.1f / wild %.1f)\n",
         bandSum / NWALKS, med(band, NWALKS), bandMin, bandMax, nT ? bandT / nT : 0, nW ? bandW / nW : 0);
  printf("band growth over one extra period: mean=%.3e max=%.3e (0 = confirmed periodic)\n",
         exs / NWALKS, exMax);
  printf("band/√W at budget cutoffs 1e3/1e4/1e5/1e6: %.1f / %.1f / %.1f / %.1f (means)\n",
         b3s / NWALKS, b4s / NWALKS, b5s / NWALKS, b6s / NWALKS);
  return 0;
}
