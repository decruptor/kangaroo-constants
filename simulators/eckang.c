// eckang.c — EC ground truth for SOTA / SOTA+ on real secp256k1 (OpenSSL), small range.
// Mirrors kangsim.c methods 3/4 exactly but with real EC point arithmetic; the SOTA+ step
// computes BOTH branches P+sJ and P-sJ from ONE inversion of (x_J - x_P), like RC's code.
// Build: clang -O2 -march=native eckang.c -o eckang -I/opt/homebrew/opt/openssl/include -L/opt/homebrew/opt/openssl/lib -lcrypto
// Run:   ./eckang -m 3|4 -t trials [-v 0|1|2] [-k kangs] [-d dpbits] [-R rbits]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/objects.h>
#include <openssl/obj_mac.h>

typedef uint64_t u64; typedef int64_t s64; typedef uint32_t u32;
static u64 g_rng = 0x1234567890ABCDEFULL;
static inline u64 mix64(u64 x){x+=0x9E3779B97F4A7C15ULL;x=(x^(x>>30))*0xBF58476D1CE4E5B9ULL;x=(x^(x>>27))*0x94D049BB133111EBULL;return x^(x>>31);}
static inline u64 rnd64(void){g_rng+=0x9E3779B97F4A7C15ULL;return mix64(g_rng);}

#define MADDR 16384
static EC_GROUP *grp; static BN_CTX *bnx;
static BIGNUM *jdist[MADDR]; static EC_POINT *jmp[MADDR];
static BIGNUM *JX[MADDR], *JY[MADDR];
static BIGNUM *ZERO;
static int RBITS = 32, DPBITS = 5, KANG = 512, METHOD = 3, SPV = 2;

typedef struct { u64 key; s64 cp; u32 role; u32 gen; } DBEnt;
static DBEnt *db; static u64 dbmask; static u32 g_gen;
static DBEnt *db_find_or_add(u64 key, u32 role, s64 cp) {
  u64 i = mix64(key + 0xD1B54A32D192ED03ULL) & dbmask;
  for (;;) {
    DBEnt *e = &db[i];
    if (e->gen != g_gen) { e->key = key; e->cp = cp; e->role = role; e->gen = g_gen; return NULL; }
    if (e->key == key) return e;
    i = (i + 1) & dbmask;
  }
}
static BIGNUM *ZERO;
static void db_init(u64 cap){ u64 c=1024; while(c<cap) c<<=1; db=malloc(sizeof(DBEnt)*c); memset(db,0,sizeof(DBEnt)*c); dbmask=c-1; }
static int sota_cands(s64 c1, int r1, s64 c2, int r2, s64 target) {
  int o1 = r1 ? -1 : 0, o2 = r2 ? -1 : 0;
  if (o1 != o2) { s64 num = c1 - c2, den = o2 - o1; if (num % den == 0 && num / den == target) return 1; }
  if (o1 + o2 != 0) { s64 num = -(c1 + c2), den = o1 + o2; if (num % den == 0 && num / den == target) return 1; }
  return 0;
}

typedef struct { EC_POINT *p; s64 dist; int role; u64 old[16]; int iter, oldn; } Kang;

int main(int argc, char **argv) {
  int trials = 20;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-m")) METHOD = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-t")) trials = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-v")) SPV = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-k")) KANG = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-d")) DPBITS = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-R")) RBITS = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-s")) g_rng = strtoull(argv[++i], 0, 16);
  }
  u64 dpmask = (1ULL << DPBITS) - 1;
  ZERO = BN_new();
  grp = EC_GROUP_new_by_curve_name(NID_secp256k1);
  bnx = BN_CTX_new();
  const BIGNUM *order = EC_GROUP_get0_field(grp); /* FIELD prime, not group order! */
  u64 minjump = 1ULL << (RBITS / 2 + 3);
  BIGNUM *t = BN_new(), *jx = BN_new(), *jy = BN_new();
  for (int i = 0; i < MADDR; i++) {
    u64 d = (minjump + (rnd64() % minjump)) & ~1ULL;
    jdist[i] = BN_new(); BN_set_word(jdist[i], (BN_ULONG)d);
    jmp[i] = EC_POINT_new(grp);
    EC_POINT_mul(grp, jmp[i], jdist[i], NULL, NULL, bnx);
    EC_POINT_get_affine_coordinates(grp, jmp[i], jx, jy, bnx);
    JX[i] = BN_new(); JY[i] = BN_new();
    BN_copy(JX[i], jx); BN_copy(JY[i], jy);
  }
  u64 x32v = 1ULL << (RBITS - 5);
  db_init(1ULL << 19);
  double sK = 0; int solvedcnt = 0;
  for (int tr = 0; tr < trials; tr++) {
    g_gen++;
    u64 kv = rnd64() % (1ULL << RBITS);
    s64 kprime = (s64)(kv + x32v);
    EC_POINT *Qp = EC_POINT_new(grp), *Wbase = EC_POINT_new(grp);
    BN_set_word(t, (BN_ULONG)(kv + x32v));
    EC_POINT_mul(grp, Qp, t, NULL, NULL, bnx);
    EC_POINT_copy(Wbase, Qp); EC_POINT_invert(grp, Wbase, bnx);
    Kang *ks = calloc(KANG, sizeof(Kang));
    for (int i = 0; i < KANG; i++) {
      Kang *kg = &ks[i]; memset(kg, 0, sizeof(Kang));
      kg->p = EC_POINT_new(grp);
      int tame = (i < KANG / 3);
      u64 d = tame ? (rnd64() % x32v) : ((rnd64() % (34 * x32v)) & ~1ULL);
      kg->dist = (s64)d; kg->role = tame ? 0 : 1;
      BN_set_word(t, (BN_ULONG)d);
      EC_POINT_mul(grp, kg->p, t, NULL, NULL, bnx);
      if (!tame) EC_POINT_add(grp, kg->p, Wbase, kg->p, bnx);
    }
    u64 ops = 0; int solved = 0;
    u64 cap = (u64)(300.0 * pow(2.0, RBITS / 2.0));
    BIGNUM *x = BN_new(), *y = BN_new(), *dx = BN_new(), *u = BN_new(),
           *xn = BN_new(), *yn = BN_new(), *s3 = BN_new();
    while (!solved && ops < cap) {
      for (int i = 0; i < KANG; i++) {
        Kang *kg = &ks[i];
        if (getenv("ECTRACE")) fprintf(stderr, "op %llu kang %d\n", (unsigned long long)ops, i);
        // loop check (compare dist low bits against previous 16) BEFORE storing
        int cycled = (kg->iter > (20 << DPBITS));
        for (int j = 0; j < 16 && j < kg->oldn && !cycled; j++)
          if ((u64)kg->dist == kg->old[j]) cycled = 1;
        kg->old[kg->oldn & 15] = (u64)kg->dist; kg->oldn++; kg->iter++;
        if (cycled) {
          int tame = (kg->role == 0);
          u64 d = tame ? (rnd64() % x32v) : ((rnd64() % (34 * x32v)) & ~1ULL);
          kg->dist = (s64)d; kg->oldn = 0; kg->iter = 0; memset(kg->old, 0, sizeof(kg->old));
          BN_set_word(t, (BN_ULONG)d);
          EC_POINT_mul(grp, kg->p, t, NULL, NULL, bnx);
          if (!tame) EC_POINT_add(grp, kg->p, Wbase, kg->p, bnx);
          continue;
        }
        EC_POINT_get_affine_coordinates(grp, kg->p, x, y, bnx);
        u32 idx = (u32)BN_mod_word(x, MADDR);
        int yodd = (int)BN_mod_word(y, 2);
        s64 jv = (s64)BN_get_word(jdist[idx]);
        // dx = x_J - x_P ; u = dx^-1 (ONE inversion serves both branches)
        BN_mod_sub(dx, JX[idx], x, order, bnx);
        BIGNUM *uu = BN_mod_inverse(u, dx, order, bnx);
        if (!uu) { /* point at infinity edge: skip step */ ops++; continue; }
        // lambda+ = (jy - y)*u ; lambda- = (-jy - y)*u
        BIGNUM *l_plus = BN_new(), *l_minus = BN_new();
        BN_mod_sub(l_plus, JY[idx], y, order, bnx);
        BN_mod_mul(l_plus, l_plus, u, order, bnx);
        BN_mod_sub(l_minus, JY[idx], y, order, bnx);
        // (-jy - y) = -(jy + y)
        BN_mod_add(s3, JY[idx], y, order, bnx);
        BN_mod_sub(l_minus, ZERO, s3, order, bnx); // -s3
        BN_mod_mul(l_minus, l_minus, u, order, bnx);
        // candidate points: for sg=+1: main=+J (l_plus), alt=-J (l_minus); sg=-1 swapped
        BIGNUM *lx[2];
        lx[0] = l_plus; lx[1] = l_minus;
        BIGNUM *xs[2], *ys[2];
        xs[0] = BN_new(); ys[0] = BN_new(); xs[1] = BN_new(); ys[1] = BN_new();
        for (int b = 0; b < 2; b++) {
          // x_b = l^2 - x - jx ; y_b = l*(x - x_b) - y
          BN_mod_sqr(xs[b], lx[b], order, bnx);
          BN_mod_sub(xs[b], xs[b], x, order, bnx);
          BN_mod_sub(xs[b], xs[b], JX[idx], order, bnx);
          BN_mod_sub(yn, x, xs[b], order, bnx);
          BN_mod_mul(yn, lx[b], yn, order, bnx);
          BN_mod_sub(ys[b], yn, y, order, bnx);
        }
        int mainsg = yodd ? -1 : 1;
        int main_idx = (mainsg > 0) ? 0 : 1, alt_idx = 1 - main_idx;
        int pick = main_idx;
        if (METHOD == 4) {
          u32 tm = (u32)BN_mod_word(xs[main_idx], 4), ta = (u32)BN_mod_word(xs[alt_idx], 4);
          int alt = 0;
          if (SPV == 0) { if (tm & 1) alt = ((ta & 1) == 0); }
          else if (SPV == 1) { if ((tm & 3) == 3) alt = ((ta & 3) != 3); }
          else alt = (ta < tm);
          if (alt) pick = alt_idx;
        }
        s64 dj = (pick == 0) ? jv : -jv; /* xs[0]=P+J, xs[1]=P-J regardless of mainsg */
        if (!EC_POINT_set_affine_coordinates(grp, kg->p, xs[pick], ys[pick], bnx)) {
          fprintf(stderr, "set_affine failed at op %llu\n", (unsigned long long)ops);
          return 3;
        }
        kg->dist += dj;
        ops++;
        int solved_here = 0;
        if ((BN_mod_word(xs[pick], dpmask + 1) == 0)) {
          u64 key;
          { // low 64 bits of x as key
            BIGNUM *tmp = BN_dup(xs[pick]);
            BN_mask_bits(tmp, 64);
            key = BN_get_word(tmp);
            BN_free(tmp);
          }
          s64 cp = kg->dist;
          DBEnt *e = db_find_or_add(key, kg->role, cp);
          if (e && !(e->role == kg->role && e->cp == cp))
            solved_here = sota_cands(cp, kg->role, e->cp, e->role, kprime);
          (void)key;
        }
        BN_free(l_plus); BN_free(l_minus);
        BN_free(xs[0]); BN_free(ys[0]); BN_free(xs[1]); BN_free(ys[1]);
        if (solved_here) { solved = 1; break; }
      }
    }
    if (!solved) printf("trial %d gave up\n", tr);
    BN_free(x); BN_free(y); BN_free(dx); BN_free(u); BN_free(xn);
    BN_free(yn); BN_free(s3);
    if (solved) { sK += (double)ops / pow(2.0, RBITS / 2.0); solvedcnt++; }
    for (int i = 0; i < KANG; i++) EC_POINT_free(ks[i].p);
    free(ks); EC_POINT_free(Qp); EC_POINT_free(Wbase);
  }
  if (solvedcnt) printf("EC method=%d v=%d R=%d kangs=%d DP=%d trials=%d: solved %d K=%.4f\n",
                        METHOD, SPV, RBITS, KANG, DPBITS, trials, solvedcnt, sK / solvedcnt);
  else printf("EC method=%d: none solved\n", METHOD);
  return 0;
}
