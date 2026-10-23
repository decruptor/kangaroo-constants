// walktest.c — diagnostic: does the SOTA adaptive-sign walk drift or diffuse?
// Compares real secp256k1 walk (OpenSSL) vs Z/p hash-oracle model.
// Walk rule (faithful to RetiredC Kang-1/RCKangaroo):
//   i = x(P) mod M            (x-coordinate low bits; same for -P)
//   s = +1 if y(P) even else -1   (antisymmetric under negation)
//   P <- P + s*J_i ; dist += s*j_i
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/objects.h>
#include <openssl/obj_mac.h>

#define M 16384
#define STEPS 100000
#define TRIALS 20
#define RBITS 32           // range bits: W = 2^RBITS, k in [0,2^RBITS)

static EC_GROUP *grp;
static BN_CTX *bnctx;
static BIGNUM *jdist[M];
static EC_POINT *jmp[M];
static EC_POINT *P;
static BIGNUM *bx, *by;

static u_int64_t rngstate = 0x123456789abcdefULL;
static u_int64_t rnd64(void) {
  rngstate += 0x9E3779B97F4A7C15ULL;
  u_int64_t z = rngstate;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

static void ec_init(void) {
  grp = EC_GROUP_new_by_curve_name(NID_secp256k1);
  bnctx = BN_CTX_new();
  bx = BN_new(); by = BN_new();
  u_int64_t minjump = 1ULL << (RBITS / 2 + 3);
  BIGNUM *t = BN_new();
  for (int i = 0; i < M; i++) {
    u_int64_t d = minjump + (rnd64() % minjump);
    d &= ~1ULL; // even
    jdist[i] = BN_new();
    BN_set_word(jdist[i], (BN_ULONG)d); // d < 2^20 fits
    jmp[i] = EC_POINT_new(grp);
    EC_POINT_mul(grp, jmp[i], jdist[i], NULL, NULL, bnctx);
  }
  BN_free(t);
  P = EC_POINT_new(grp);
}

// returns final dist (signed), fills maxabs
static long long ec_walk(u_int64_t t0, double *maxabs, int *signflips) {
  BIGNUM *d = BN_new();
  BN_set_word(d, (BN_ULONG)t0);
  EC_POINT_mul(grp, P, d, NULL, NULL, bnctx);
  long long dist = 0;
  *maxabs = 0; *signflips = 0;
  int lastsign = 0;
  for (int s = 0; s < STEPS; s++) {
    EC_POINT_get_affine_coordinates(grp, P, bx, by, bnctx);
    unsigned int idx = (unsigned int)(BN_mod_word(bx, M));
    int sign = BN_is_odd(by) ? -1 : 1;
    if (lastsign && sign != lastsign) (*signflips)++;
    lastsign = sign;
    const BIGNUM *jd = jdist[idx];
    long long j = BN_get_word(jd);
    if (sign > 0) { EC_POINT_add(grp, P, jmp[idx], P, bnctx); dist += j; }
    else { EC_POINT *neg = EC_POINT_new(grp); EC_POINT_copy(neg, jmp[idx]); EC_POINT_invert(grp, neg, bnctx);
           EC_POINT_add(grp, P, neg, P, bnctx); EC_POINT_free(neg); dist -= j; }
    double a = fabs((double)dist);
    if (a > *maxabs) *maxabs = a;
  }
  BN_free(d);
  return dist;
}

// ---------- Z/p model ----------
#define P61 2305843009213693951ULL /* 2^61-1 */
static u_int64_t zp_jdist[M];

static inline u_int64_t mix64(u_int64_t x) {
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}
static inline u_int64_t canon(u_int64_t x) { return (x > P61 - x) ? P61 - x : x; }

static long long zp_walk(u_int64_t t0, double *maxabs, int *signflips) {
  u_int64_t x = t0 % P61;
  long long dist = 0;
  *maxabs = 0; *signflips = 0;
  int lastsign = 0;
  for (int s = 0; s < STEPS; s++) {
    u_int64_t c = canon(x);
    unsigned int idx = (unsigned int)(mix64(c ^ 0xA5A5A5A5A5A5A5A5ULL) % M);
    int sign = ((mix64(c ^ 0x5A5A5A5A5A5A5A5AULL) & 1) ^ (x != c)) ? -1 : 1;
    if (lastsign && sign != lastsign) (*signflips)++;
    lastsign = sign;
    long long j = zp_jdist[idx];
    if (sign > 0) { x += j; if (x >= P61) x -= P61; dist += j; }
    else { x += P61 - j; if (x >= P61) x -= P61; dist -= j; }
    double a = fabs((double)dist);
    if (a > *maxabs) *maxabs = a;
  }
  return (long long)x, dist;
}

int main(void) {
  u_int64_t minjump = 1ULL << (RBITS / 2 + 3);
  // init zp jump table (same distribution)
  for (int i = 0; i < M; i++) {
    u_int64_t d = minjump + (rnd64() % minjump);
    zp_jdist[i] = d & ~1ULL;
  }
  double jmean = 12.0 * (double)(1ULL << (RBITS / 2));

  printf("Walk diagnostic: RBITS=%d M=%d steps=%d trials=%d meanjump=%.3e\n",
         RBITS, M, STEPS, TRIALS, jmean);
  printf("expected final |dist| if diffusion: %.3e ; if full drift: %.3e\n",
         jmean * sqrt((double)STEPS), jmean * (double)STEPS);

  ec_init();
  double e_sum = 0, e_max = 0, e_abs = 0; double e_flips = 0;
  for (int tr = 0; tr < TRIALS; tr++) {
    u_int64_t t0 = rnd64() % (1ULL << (RBITS - 5));
    double ma; int fl;
    long long d = ec_walk(t0, &ma, &fl);
    e_sum += d; e_abs += fabs((double)d); if (ma > e_max) e_max = ma; e_flips += fl;
  }
  printf("EC  : mean dist %+.3e  mean|dist| %.3e  max|dist| %.3e  signflips/step %.3f\n",
         e_sum / TRIALS, e_abs / TRIALS, e_max, e_flips / (TRIALS * (double)STEPS));

  double z_sum = 0, z_max = 0, z_abs = 0, z_flips = 0;
  for (int tr = 0; tr < TRIALS; tr++) {
    u_int64_t t0 = rnd64() % (1ULL << (RBITS - 5));
    double ma; int fl;
    long long d = zp_walk(t0, &ma, &fl);
    z_sum += d; z_abs += fabs((double)d); if (ma > z_max) z_max = ma; z_flips += fl;
  }
  printf("Z/p : mean dist %+.3e  mean|dist| %.3e  max|dist| %.3e  signflips/step %.3f\n",
         z_sum / TRIALS, z_abs / TRIALS, z_max, z_flips / (TRIALS * (double)STEPS));
  return 0;
}
