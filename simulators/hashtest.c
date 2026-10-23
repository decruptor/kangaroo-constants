#include <stdio.h>
#include <stdint.h>
typedef uint64_t u64;
static inline u64 mix64(u64 x){x+=0x9E3779B97F4A7C15ULL;x=(x^(x>>30))*0xBF58476D1CE4E5B9ULL;x=(x^(x>>27))*0x94D049BB133111EBULL;return x^(x>>31);}
#define PMOD 2305843009213693951ULL
static u64 canon(u64 x){u64 y=PMOD-x;return x<y?x:y;}
int main(){
  u64 c0=0,c31=0; u64 N=10000000;
  for(u64 i=1;i<=N;i++){ u64 e=(i*2654435761)%4000000000ULL; u64 c=canon(e);
    u64 h=mix64(c ^ 0x3333333333333333ULL);
    if((h&31)==0)c0++; if((h&0xFFFFF)==0)c31++;
  }
  printf("rate&31=1/%.2f rate&0xFFFFF=1/%.2f\n",(double)N/c0,(double)N/c31);
  return 0;
}
