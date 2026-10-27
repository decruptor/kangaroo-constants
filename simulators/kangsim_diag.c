// kangsim.c — empirical validation of Pollard-kangaroo walk constants (Z/p hash-oracle model)
//
// Faithful CPU port of RetiredC's Kang-1 experiment (ExpKangDlg.cpp), which defines:
//   0 classic1v1 : textbook 1 tame + 1 wild, directed jumps, every-point DB (ideal K, no DP overhead)
//   1 classic    : Kang-1 "Classic" herd (half tame / half wild, directed jumps, DP DB)
//   2 3way       : Kang-1 "3-Way" (tame/wild1/wild2, wild2 = negated base, even jumps)
//   3 sota       : Kang-1 "SOTA" (= SOTAv2 family: 1/3 tames in [0,W/32), 2/3 wilds even in
//                  [0,1.0625W) at -Q', Q'=Q+W/32; adaptive +-sign jumps; DP DB; simple loop handling)
//   4 sotap      : SOTA+ = SOTA + one-step lookahead: compute both P+J and P-J (same
//                  inversion/denominator), continue from the better one (RC's rule variants)
//   method 3 or 4 with -mirror: the "second walking point" reading of SOTA+ (each kang also
//                  advances the mirror trail Y = c - X; Y step = negated jump; 0 or 0.3 op/step)
//
// Group model: additive Z/p, p = 2^61-1. Element = true exponent E (|E| << p, no wraparound);
// negation is E -> -E. EC "x-coordinate low bits" oracles are modeled as hash bits of the
// canonical form canon(E)=min(E,p-E) (which is +-invariant, like the EC x-coordinate); the EC
// "y-parity" jump-sign bit is modeled as an antisymmetric hash bit: signbit(-E)=1-signbit(E).
// Op counter: 1 per point addition (= one jump step of one kang); restarts cost 0 (as in Kang-1).
//
// Collision algebra: a stored record is (cp, o) with true exponent E = cp + o*kbase,
// kbase = k' = k + W/32 for the SOTA family, kbase = k for classic/3way; o = 0 (tame-role),
// +1 (wild at +Q'), -1 (wild at -Q'). A collision (equal or negated points) between records
// 1,2 yields:  E1=E2  -> kbase=(cp1-cp2)/(o2-o1)  (o1!=o2);   E1=-E2 -> kbase=-(cp1+cp2)/(o1+o2).
// Kang-1's per-method candidate sets are subsets of this (checked by hand against the source).
//
// Build: clang -O3 -march=native kangsim.c -o kangsim -lm
// Run:   ./kangsim -m 3 -n 41 -t 200
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
static u64 g_trng; static inline u64 trnd64(void) { g_trng += 0x9E3779B97F4A7C15ULL; return mix64(g_trng); }
static inline u64 trndmax(u64 r) { return trnd64() % r; }

static inline u64 canon(u64 x) { u64 y = PMOD - x; return x < y ? x : y; }
static inline u64 padd(u64 a, u64 b) { u64 s = a + b; if (s >= PMOD) s -= PMOD; return s; }
static inline u64 psub(u64 a, u64 b) { return a >= b ? a - b : a + (PMOD - b); }
static inline u64 modn(s64 v) { s64 m = v % (s64)PMOD; if (m < 0) m += (s64)PMOD; return (u64)m; }

// ---------------- parameters ----------------
static int METHOD = 3, NBITS = 41, TRIALS = 200, KANG = 512, DPBITS = 5, MLOG = 14;
static int SP_VARIANT = 2;   // sotap rule: 0=v1(K~1.02) 1=lite(K~1.05) 2=gpu(K~0.99)
static int W_ALT = 0;        // cost of computing the alternative point, in 1/1000 op
static int MIRROR = 0;       // also advance mirror trail Y = c - X each step
static int JUMP_SHIFT = 3;   // minjump = 2^(R/2 + JUMP_SHIFT)  (RC uses +3)
static double JMEAN_MUL = 2.0; // classic1v1: mean jump = JMEAN_MUL*sqrt(W)
static u64 g_seed = 0xABCDEF0123456789ULL;
static u64 g_tseed = 0;               // separate jump-table seed (-ts); 0 = share g_seed

#define MMASK ((1u << MLOG) - 1)

// ---------------- per-run globals ----------------
static u64 W; static int RR; static u64 X32; static double SQRTW;
static u64 jmpdist[1 << 20];
static u64 jmpdist2[1 << 20];
static int USE_ESC = 0;
static u64 dpmask; static s64 max_iters;

// DB: open addressing with generation tag; key = canon | (role<<61); value = cp (code')
typedef struct { u64 key; s64 cp; u32 role; u32 gen; } DBEnt;
static DBEnt *db; static u64 dbmask; static u32 g_gen;
#define R_TAME 0u   // o = 0
#define R_WILD 1u   // o = -1 (SOTA) or +1 (classic/3way wild1) -- see cpo
#define R_WILDP 2u  // o = +1 (SOTA mirror / 3way wild2)

static void db_init(u64 cap) {
  u64 c = 1024; while (c < cap) c <<= 1;
  db = (DBEnt*)malloc(sizeof(DBEnt) * c);
  memset(db, 0, sizeof(DBEnt) * c);
  dbmask = c - 1;
}
static DBEnt *db_find_or_add(u64 canonv, u32 role, s64 cp) {
  u64 i = mix64(canonv + 0xD1B54A32D192ED03ULL) & dbmask;
  for (;;) {
    DBEnt *e = &db[i];
    if (e->gen != g_gen) { e->key = canonv; e->cp = cp; e->role = role; e->gen = g_gen; return NULL; }
    if (e->key == canonv) return e;
    i = (i + 1) & dbmask;
  }
}

// oracles: in EC the jump index (x mod M), the DP test (x & DPmask) and the SOTA+ branch
// tag (x & 3) are ALL low bits of the SAME x-coordinate => strongly correlated. We model
// the x-coordinate low bits with ONE hash of the canonical form (+-invariant).
static inline u64 el_H(u64 e) { return mix64(canon(e) ^ 0x1111111111111111ULL); }
static inline u64 el_H_raw(u64 e) { return mix64(e ^ 0x1111111111111111ULL); }
static inline u64 el_index(u64 e) { return el_H(e) & MMASK; }
static inline int el_signbit(u64 e) { // +1 if "y even" else -1; antisymmetric: sign(-e)=-sign(e)
  u64 c = canon(e);
  return ((mix64(c ^ 0x2222222222222222ULL) & 1) ^ (u64)(e != c)) ? -1 : 1;
}
static inline int el_isdp(u64 e) { return ((el_H(e) & dpmask) == 0); }
static inline u32 el_tag(u64 e) { return (u32)(el_H(e) & 3); }
// classic methods: no symmetry used, oracles of the raw element
static inline u64 el_index_raw(u64 e) { return el_H_raw(e) & MMASK; }
static inline int el_isdp_raw(u64 e) { return ((el_H_raw(e) & dpmask) == 0); }

typedef struct {
  u64 x; s64 dist; u32 role; s64 cpo;      // cp = dist + cpo at DP time; E = cp + o*kbase
  s64 mc; s64 mcode; u32 mrole;            // mirror trail: cp_m = mcode; E_m = mcode + o_m*kbase
  s64 old[16]; int iter, oldn;
  int esc;                                 // pending escape (use table2 next step, advanced loop handling)
  u64 nxj; int nsg;                        // lookahead: index and sign at the CURRENT point
  // -cyclog2: online Brent cycle oracle over the deterministic map state (x, esc).
  // The walk is a deterministic map on (x, esc), so x == tortoise (with equal esc)
  // means the kangaroo is ON a cycle; the flag is monotone within a life.
  u64 bx; u32 bstep, bnext; int incyc; int besc; u32 bxs; int cycseen;
} Kang;

static u64 g_ins, g_restarts, g_steps;
static int g_cyclog = 0; static u64 g_cyc_kang, g_cyc_samples, g_cyc_trials;
static u64 g_match_tw, g_match_ww, g_match_tt, g_match_other, g_cand_near;
static s64 g_bestdiff; static double g_maxdisp;
// -cyclog2: long-cycle (lambda > 16) instrumentation
static int g_cyclog2 = 0;
static u64 g_lives_short, g_lives_to_cycle, g_lives_to_free;   // life-end classification
static u64 g_lives_exitcyc;   // lives terminated by herd exit while in a long cycle
static u64 g_steps_incyc;                                      // steps taken while in a long cycle
static u64 g_trapped26;                                        // kangs in-cycle at life-step 26k
static u64 g_bstep;                                            // max life-step reached (any kang)
#define NBKT 14
static const u32 BKT[NBKT] = {1000,2000,4000,8000,16000,26000,32000,64000,128000,256000,512000,
                              1000000,2000000,4000000};
static u64 g_bkt_kang[NBKT], g_bkt_incyc[NBKT];                // occupancy by life-step bucket
// solve channel (which collision class produced the SOLVING match)
static u64 g_solve_tw, g_solve_ww, g_solve_other;
// first-fire diagnostics: gap between snapshot and first Brent hit
static u64 g_fire1, g_gap[4], g_fire_sd, g_endcyc_short;

static inline void push_old(Kang *kg) { kg->old[kg->oldn & 15] = kg->dist; kg->oldn++; }
static inline int detect_loop(Kang *kg) {
  for (int i = 0; i < 16 && i < kg->oldn; i++) if (kg->old[i] == kg->dist) return 1;
  return 0;
}

static int sota_cands(s64 c1, u32 r1, s64 c2, u32 r2, s64 target) {
  int o1 = (r1 == R_WILD) ? -1 : (r1 == R_WILDP) ? 1 : 0;
  int o2 = (r2 == R_WILD) ? -1 : (r2 == R_WILDP) ? 1 : 0;
  if (o1 != o2) { s64 num = c1 - c2, den = o2 - o1;
    if (num % den == 0 && num / den == target) return 1; }
  if (o1 + o2 != 0) { s64 num = -(c1 + c2), den = o1 + o2;
    if (num % den == 0 && num / den == target) return 1; }
  return 0;
}
// classic/3way: RC checks only the "same direction" channel
static int dir_cand(s64 c1, u32 r1, s64 c2, u32 r2, s64 target) {
  int o1 = (r1 == R_TAME) ? 0 : ((r1 == R_WILD) ? 1 : -1);
  int o2 = (r2 == R_TAME) ? 0 : ((r2 == R_WILD) ? 1 : -1);
  if (o1 == o2) return 0;
  s64 num = c1 - c2, den = o2 - o1;
  if (num % den) return 0;
  return num / den == target;
}

#define GIVEUP (1ULL << 49)

// set up a kang's start (method 1..4); returns nothing; kprime used by sota family
static void init_kang(Kang *kg, int method, int tame, int wtype, u64 k, u64 kprime) {
  memset(kg, 0, sizeof(Kang));
  if (method == 1) {
    s64 d = (s64)rndmax(W);
    kg->dist = d; kg->cpo = 0; kg->role = R_TAME;
    kg->x = modn(d);
    if (!tame) { kg->role = R_WILD; kg->cpo = -((s64)1 << (RR - 1));
      kg->x = padd(modn(d), modn((s64)k - ((s64)1 << (RR - 1)))); }
  } else if (method == 2) {
    if (tame) {
      s64 d = (s64)rndmax(1ULL << (RR - 4));
      kg->dist = d; kg->cpo = ((s64)1 << (RR - 1)) - ((s64)1 << (RR - 5)); // +W/2-W/32
      kg->x = modn(d + kg->cpo); kg->role = R_TAME;
    } else {
      s64 d = (s64)(rndmax(W) & ~1ULL);
      kg->dist = d;
      u64 base = wtype ? modn(((s64)1 << (RR - 1)) - (s64)k) : modn((s64)k - ((s64)1 << (RR - 1)));
      kg->x = padd(base, modn(d));
      kg->cpo = wtype ? ((s64)1 << (RR - 1)) : -((s64)1 << (RR - 1));
      kg->role = wtype ? R_WILDP : R_WILD;
    }
  } else if (method == 6) { // Kang-1 "Mirror": adaptive-sign walk, 2 groups, wild base Q-W/2
    if (tame) {
      s64 d = (s64)rndmax((u64)9 << (RR - 4)); // [0, 9W/16)
      kg->dist = d; kg->x = modn(d); kg->role = R_TAME; kg->cpo = 0;
    } else {
      s64 d = (s64)rndmax((u64)1 << (RR - 4)); // [0, W/16), any parity
      kg->dist = d; kg->x = padd(modn(d), modn((s64)k - ((s64)1 << (RR - 1))));
      kg->role = R_WILDP; kg->cpo = -((s64)1 << (RR - 1)); /* E = cp + k */
    }
  } else { // sota family
    if (tame) {
      s64 d = (s64)rndmax(X32);
      kg->dist = d; kg->x = modn(d); kg->role = R_TAME; kg->cpo = 0;
    } else {
      s64 d = (s64)(rndmax((u64)34 * X32) & ~1ULL);
      kg->dist = d; kg->x = psub(modn(d), kprime); kg->role = R_WILD; kg->cpo = 0;
    }
  }
  if (MIRROR && method >= 3) {
    if (kg->role == R_TAME) {
      s64 d2 = (s64)rndmax(X32);
      kg->mc = kg->dist + d2; kg->mcode = d2; kg->mrole = R_TAME;
    } else {
      s64 d2 = (s64)(rndmax((u64)34 * X32) & ~1ULL);
      s64 E0 = kg->dist - (s64)kprime;
      kg->mc = E0 + (d2 - (s64)kprime);
      kg->mcode = kg->mc - E0; kg->mrole = R_WILDP;
    }
  }
}

static int run_herd(u64 k, int method, double *ops_out, double *ops2_out) {
  g_gen++;
  u64 kprime = k + X32;
  Kang *ks = (Kang*)calloc(KANG, sizeof(Kang));
  for (int i = 0; i < KANG; i++) {
    int tame, wtype = 0;
    if (method == 1 || method == 6) tame = (i < KANG / 2);
    else if (method == 2) { tame = (i < KANG / 3); wtype = (i >= 2 * KANG / 3); }
    else tame = (i < KANG / 3);
    init_kang(&ks[i], method, tame, wtype, k, kprime);
  }
  u64 ops = 0, wops = 0;
  s64 target = (method == 1 || method == 2) ? (s64)k : (method == 6 ? (s64)k : (s64)kprime);
  int solved = 0;
  u64 cap = (u64)(200.0 * SQRTW);
  while (!solved && ops < cap) {
    for (int i = 0; i < KANG; i++) {
      Kang *kg = &ks[i];
      int sym = (method >= 3); (void)sym;
      if (sym) { // Kang-1 simple loop handling (check BEFORE storing current dist)
        kg->iter++;
        int sd = detect_loop(kg);
        int cycled = sd || kg->iter > max_iters;
        push_old(kg);
        if (cycled) {
          g_restarts++;
          if (g_cyclog2) { // life-end classification: short-detect vs timeout, and
                           // whether the timeout-ended life was inside a long cycle
            if (sd) { g_lives_short++; if (kg->incyc) g_endcyc_short++; }
            else if (kg->incyc) g_lives_to_cycle++;
            else g_lives_to_free++;
          }
          init_kang(kg, method, kg->role == R_TAME, kg->role == R_WILDP, k, kprime);
          continue;
        }
      }
      s64 dprev = kg->dist;
      if (!sym) {
        u64 j = jmpdist[el_index_raw(kg->x)];
        kg->x = padd(kg->x, j); kg->dist += (s64)j; ops++; g_steps++;
      } else {
        u64 j; int sg; u64 cur_i = el_index(kg->x);
        if (USE_ESC && kg->esc) { j = jmpdist2[kg->nxj]; sg = kg->nsg; kg->esc = 0; }
        else { j = jmpdist[cur_i]; sg = el_signbit(kg->x); }
        u64 x1 = (sg > 0) ? padd(kg->x, j) : psub(kg->x, j);
        u64 x2 = (sg > 0) ? psub(kg->x, j) : padd(kg->x, j);
        if (method == 4) {
          int alt = 0;
          if (SP_VARIANT == 0) { if (el_tag(x1) & 1) { wops += W_ALT; alt = ((el_tag(x2) & 1) == 0); } }
          else if (SP_VARIANT == 1) { if ((el_tag(x1) & 3) == 3) { wops += W_ALT; alt = ((el_tag(x2) & 3) != 3); } }
          else { wops += W_ALT; alt = (el_tag(x2) < el_tag(x1)); }
          if (alt) { kg->x = x2; kg->dist = dprev + (sg > 0 ? -(s64)j : (s64)j); }
          else { kg->x = x1; kg->dist = dprev + (sg > 0 ? (s64)j : -(s64)j); }
        } else {
          kg->x = x1; kg->dist = dprev + (sg > 0 ? (s64)j : -(s64)j);
        }
        ops++; g_steps++;
        if (USE_ESC) { // detect L1S2 bounce: the jump FROM the new point would exactly undo this one
          kg->nxj = el_index(kg->x); kg->nsg = el_signbit(kg->x);
          kg->esc = (kg->nxj == cur_i && kg->nsg == -sg);
        }
      }
      if (g_cyclog2 && sym) { // online Brent over the deterministic map state (x, esc)
        kg->bstep++;
        if (kg->bnext == 0) { kg->bx = kg->x; kg->besc = kg->esc; kg->bnext = 2; kg->bxs = kg->bstep; }
        else if (kg->bstep == kg->bnext) { kg->bx = kg->x; kg->besc = kg->esc; kg->bnext <<= 1; kg->bxs = kg->bstep; }
        else if (kg->x == kg->bx && kg->esc == kg->besc) {
          kg->incyc = 1;
          if (!kg->cycseen) { kg->cycseen = 1; g_fire1++;
            u64 gap = kg->bstep - kg->bxs;
            g_gap[gap <= 16 ? 0 : gap <= 256 ? 1 : gap <= 65536 ? 2 : 3]++;
            if (detect_loop(kg)) g_fire_sd++;
          }
        }
        if (kg->incyc) g_steps_incyc++;
        if (kg->bstep <= 4000000u)
          for (int b = 0; b < NBKT; b++)
            if (kg->bstep == BKT[b]) { g_bkt_kang[b]++; g_bkt_incyc[b] += (kg->incyc != 0); }
      }
      int solved_here = 0;
      // mirror trail DP check (no extra group op; extra hash only)
      if (MIRROR && sym) {
        wops += W_ALT; // second point advance costs W_ALT/1000 ops (shared inversion)
        kg->mcode -= (kg->dist - dprev);
        u64 xm = modn(kg->mcode + (kg->mrole == R_WILDP ? (s64)kprime : 0));
        if (el_isdp(xm)) {
          g_ins++;
          DBEnt *e = db_find_or_add(canon(xm), kg->mrole, kg->mcode);
          if (e) solved_here = sota_cands(kg->mcode, kg->mrole, e->cp, e->role, target);
          else { kg->iter = 0; kg->oldn = 0; memset(kg->old, 0, sizeof(kg->old)); }
        }
      }
      if (!solved_here && el_isdp(kg->x)) {
        g_ins++;
        s64 cp = kg->dist + kg->cpo;
        DBEnt *e = db_find_or_add(canon(kg->x), kg->role, cp);
        if (e) {
          u32 r2 = e->role;
          u32 pr = (kg->role<<2)|r2;
          if (pr==((R_TAME<<2)|R_WILD)||pr==((R_WILD<<2)|R_TAME)) g_match_tw++;
          else if ((pr&3)==R_WILD && (pr>>2)==R_WILD) g_match_ww++;
          else if (pr==((R_TAME<<2)|R_TAME)) g_match_tt++;
          else g_match_other++;
          { s64 c1=cp, c2=e->cp; s64 d1=c1-c2, d2v=c1+c2;
            s64 bd=0; int first=1;
            if (kg->role!=r2) { s64 v=d1; if(v<0)v=-v; if(first||v<bd){bd=v;first=0;} 
              if(d1 % ((r2==R_WILD)?-1:1)==0){} }
            if (!((kg->role==R_TAME&&r2==R_TAME))) { s64 v=d2v; if(v<0)v=-v; if(first||v<bd){bd=v;first=0;} }
            if (!first && (g_bestdiff==0 || bd<g_bestdiff)) g_bestdiff=bd; }
          int ok;
          if (method == 1 || method == 2) ok = ((kg->role != r2) && dir_cand(cp, kg->role, e->cp, r2, target));
          else if (method == 6) ok = ((kg->role != r2) && sota_cands(cp, kg->role, e->cp, r2, target));
          else ok = (!(kg->role == r2 && cp == e->cp)) && sota_cands(cp, kg->role, e->cp, r2, target);
          if (ok) {
            solved_here = 1;
            // solve channel: which collision class fired the SOLVING match
            if ((kg->role == R_WILD || kg->role == R_WILDP) && (r2 == R_WILD || r2 == R_WILDP)) g_solve_ww++;
            else if ((kg->role == R_TAME) != (r2 == R_TAME)) g_solve_tw++;
            else g_solve_other++;
          }
        } else { kg->iter = 0; kg->oldn = 0; memset(kg->old, 0, sizeof(kg->old)); }
      }
      if (solved_here) { solved = 1; break; }
    }
  }
  { double md=0; for (int i=0;i<KANG;i++){ double a=fabs((double)ks[i].dist); if(a>md) md=a; }
    if (md>g_maxdisp) g_maxdisp=md; }
  if (g_cyclog) {
    int ncyc = 0;
    for (int i = 0; i < KANG; i++) if (detect_loop(&ks[i])) ncyc++;
    g_cyc_kang += ncyc; g_cyc_samples += KANG; g_cyc_trials += 1;
  }
  if (g_cyclog2) // lives terminated by herd exit (solve or cap) while inside an
                 // undetected long cycle: never classified by the restart path
    for (int i = 0; i < KANG; i++) if (ks[i].bstep && ks[i].incyc) g_lives_exitcyc++;
  free(ks);
  *ops_out = (double)ops;
  *ops2_out = (double)ops + (double)wops / 1000.0;
  return solved;
}

// ---- classic 1v1, ideal (every visited point checked) ----
static int run_1v1(u64 k, double *ops_out) {
  g_gen++;
  s64 t0 = (s64)rndmax(W);
  u64 tx = modn(t0); s64 tdist = t0; // tame tracked = true exponent t0 + A
  u64 wx = modn((s64)k); s64 wdist = 0;              // wild tracked = displacement from Q
  u64 ops = 0;
  u64 cap = (u64)(400.0 * SQRTW);
  // store every visited point of each kang; collision = point of one in the other's set
  for (;;) {
    u64 j = jmpdist[el_index_raw(tx)];
    tx = padd(tx, j); tdist += (s64)j; ops++;
    DBEnt *e = db_find_or_add(tx, R_TAME, tdist);
    if (e && e->role == R_WILD && tdist - e->cp == (s64)k) { *ops_out = (double)ops; return 1; }
    if (ops >= cap) return 0;
    j = jmpdist[el_index_raw(wx)];
    wx = padd(wx, j); wdist += (s64)j; ops++;
    e = db_find_or_add(wx, R_WILD, wdist);
    if (e && e->role == R_TAME && e->cp - wdist == (s64)k) { *ops_out = (double)ops; return 1; }
  }
}

static void build_jumptable2(void) {
  u64 minjump2 = 1ULL << (RR - 10); // RCKangaroo: large jumps for loop escapes
  for (u64 i = 0; i <= MMASK; i++) {
    u64 d = minjump2 + rndmax(minjump2);
    jmpdist2[i] = (d & ~1ULL) % PMOD;
  }
}
static void build_jumptable(int even) {
  u64 minjump = 1ULL << (RR / 2 + JUMP_SHIFT);
  for (u64 i = 0; i <= MMASK; i++) {
    u64 d = minjump + rndmax(minjump);
    if (even) d &= ~1ULL;
    jmpdist[i] = d % PMOD;
  }
}

#include <time.h>
int main(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-m")) METHOD = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-n")) NBITS = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-t")) TRIALS = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-k")) KANG = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-d")) DPBITS = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-M")) MLOG = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-v")) SP_VARIANT = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-w")) W_ALT = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-mirror")) MIRROR = 1;
    else if (!strcmp(argv[i], "-esc")) USE_ESC = 1;
    else if (!strcmp(argv[i], "-jm")) JMEAN_MUL = atof(argv[++i]);
    else if (!strcmp(argv[i], "-js")) JUMP_SHIFT = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-s")) g_seed = strtoull(argv[++i], 0, 16);
    else if (!strcmp(argv[i], "-ts")) g_tseed = strtoull(argv[++i], 0, 16);
    else if (!strcmp(argv[i], "-cyclog")) g_cyclog = 1;
    else if (!strcmp(argv[i], "-cyclog2")) g_cyclog2 = 1;
  }
  RR = NBITS - 1;
  W = 1ULL << RR;
  X32 = 1ULL << (RR - 5);
  SQRTW = pow(2.0, RR / 2.0);
  dpmask = (1ULL << DPBITS) - 1;
  max_iters = (s64)20 << DPBITS;
  db_init(METHOD == 0 ? (1ULL << 23) : (1ULL << 21));
  g_rng = g_seed;
  g_trng = g_tseed ? g_tseed : g_seed;

  const char *names[] = { "classic1v1", "classic-herd", "3-way", "SOTA", "SOTA+", "mirror" };
  printf("# method=%s n=%d (W=2^%d) kang=%d DP=%d M=2^%d trials=%d v=%d walt=%d/1000 mirror=%d jm=%.2f\n",
         names[METHOD], NBITS, RR, KANG, DPBITS, MLOG, TRIALS, SP_VARIANT, W_ALT, MIRROR, JMEAN_MUL);

  build_jumptable(METHOD >= 2 && METHOD != 6);
  if (USE_ESC) build_jumptable2();
  if (METHOD == 0) { // mean jump = JMEAN_MUL*sqrt(W), uniform [a,2a)
    u64 a = (u64)(JMEAN_MUL * SQRTW / 1.5);
    for (u64 i = 0; i <= MMASK; i++) jmpdist[i] = (a + (g_tseed ? trndmax(a) : rndmax(a))) % PMOD;
  }
  double *K1 = malloc(sizeof(double) * TRIALS), *K2 = malloc(sizeof(double) * TRIALS);
  int solvedcnt = 0; double s1 = 0, s2 = 0, s12 = 0;
  for (int tr = 0; tr < TRIALS; tr++) {
    u64 k = rndmax(W);
    double ops = 0, ops2 = 0; int ok;
    if (METHOD == 0) ok = run_1v1(k, &ops), ops2 = ops;
    else ok = run_herd(k, METHOD, &ops, &ops2);
    if (!ok) { printf("trial %d: GAVE UP (censored)\n", tr); continue; }
    K1[solvedcnt] = ops / SQRTW; K2[solvedcnt] = ops2 / SQRTW;
    s1 += K1[solvedcnt]; s2 += K2[solvedcnt]; s12 += K1[solvedcnt] * K1[solvedcnt];
    solvedcnt++;
  }
  if (g_cyclog && g_cyc_trials)
    printf("cyclog: kangaroos-cycling-at-solve = %.1f%% (%llu/%llu over %llu trials)\n",
           100.0 * (double)g_cyc_kang / (double)g_cyc_samples,
           (unsigned long long)g_cyc_kang, (unsigned long long)g_cyc_samples,
           (unsigned long long)g_cyc_trials);
  if (g_cyclog2) {
    u64 lives = g_lives_short + g_lives_to_cycle + g_lives_to_free;
    printf("cyclog2 lives: short-detect=%llu (%.2f%%) timeout-in-long-cycle=%llu (%.2f%%) timeout-free=%llu (%.2f%%) exit-while-in-cycle=%llu  total=%llu\n",
           (unsigned long long)g_lives_short, 100.0 * (double)g_lives_short / (double)lives,
           (unsigned long long)g_lives_to_cycle, 100.0 * (double)g_lives_to_cycle / (double)lives,
           (unsigned long long)g_lives_to_free, 100.0 * (double)g_lives_to_free / (double)lives,
           (unsigned long long)g_lives_exitcyc,
           (unsigned long long)(lives + g_lives_exitcyc));
    printf("cyclog2 steps-in-long-cycle / total steps = %.3f%%\n",
           100.0 * (double)g_steps_incyc / (double)g_steps);
    printf("cyclog2 first-fires=%llu gap<=16:%llu <=256:%llu <=65536:%llu >65536:%llu  fired-in-detect-window=%llu  lives-ended-short-while-incyc=%llu\n",
           (unsigned long long)g_fire1, (unsigned long long)g_gap[0], (unsigned long long)g_gap[1],
           (unsigned long long)g_gap[2], (unsigned long long)g_gap[3],
           (unsigned long long)g_fire_sd, (unsigned long long)g_endcyc_short);
    printf("cyclog2 occupancy (fraction of kangaroos in an undetected long cycle, by life-step):");
    for (int b = 0; b < NBKT; b++)
      if (g_bkt_kang[b]) printf(" %u:%.3f", BKT[b], (double)g_bkt_incyc[b] / (double)g_bkt_kang[b]);
    printf("\n");
  }
  printf("matches: tw=%llu ww=%llu tt=%llu other=%llu restarts=%llu steps=%llu\n",
         (unsigned long long)g_match_tw,(unsigned long long)g_match_ww,(unsigned long long)g_match_tt,
         (unsigned long long)g_match_other,(unsigned long long)g_restarts,(unsigned long long)g_steps);
  if (solvedcnt) {
    double m1 = s1 / solvedcnt, m2 = s2 / solvedcnt;
    double var = s12 / solvedcnt - m1 * m1; if (var < 0) var = 0;
    double sd = sqrt(var), ci = 1.96 * sd / sqrt((double)solvedcnt);
    for (int i = 0; i < solvedcnt; i++) for (int j = i + 1; j < solvedcnt; j++)
      if (K1[j] < K1[i]) { double t = K1[i]; K1[i] = K1[j]; K1[j] = t; }
    printf("solved %d/%d  K_main=%.4f +/- %.4f (95%%CI)  median=%.4f  sd=%.4f\n",
           solvedcnt, TRIALS, m1, ci, K1[solvedcnt / 2], sd);
    if (METHOD == 4 || MIRROR) printf("K_weighted(alt=%d/1000)=%.4f\n", W_ALT, m2);
    printf("dp_rate=1/%.1f  ",(double)g_steps/(double)(g_ins?g_ins:1));
    printf("matches: tame-wild=%llu wild-wild=%llu tame-tame=%llu other=%llu  best |cand-k'|=%lld  maxdisp=%.3e (sqrtW=%.3e)\n",
           (unsigned long long)g_match_tw,(unsigned long long)g_match_ww,(unsigned long long)g_match_tt,(unsigned long long)g_match_other,
           (long long)g_bestdiff, g_maxdisp, SQRTW);
    printf("solves: tame-wild=%llu wild-wild=%llu other=%llu  (ww share %.1f%%)\n",
           (unsigned long long)g_solve_tw, (unsigned long long)g_solve_ww, (unsigned long long)g_solve_other,
           100.0 * (double)g_solve_ww / (double)(g_solve_tw + g_solve_ww + g_solve_other));
    printf("ops_mean=%.0f (2^%.2f)  DPs_per_kang=%.2f  restarts/kang=%.3f\n",
           m1 * SQRTW, log2(m1 * SQRTW), (double)g_ins / ((double)solvedcnt * KANG),
           (double)g_restarts / ((double)solvedcnt * KANG));
    if (METHOD >= 3) {
      double dpk = (1.15 * SQRTW) / KANG / (double)(1ULL << DPBITS);
      double Krc = 1.15 + (0.07 + 0.76 / sqrt(dpk)) / (1 + 0.30 * dpk);
      printf("RC formula: DPs_per_kang(ideal)=%.3f -> K=%.4f\n", dpk, Krc);
    }
  }
  return 0;
}
