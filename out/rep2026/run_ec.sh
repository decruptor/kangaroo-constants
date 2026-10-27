#!/bin/zsh
OUT="$(cd "$(dirname "$0")" && pwd)"
cd "$OUT/../../simulators"
for seed in EC31A001 EC31A002 EC31A003 EC31A004; do
  [ -f "$OUT/.EC_$seed.done" ] && continue
  ./eckang -m 4 -v 2 -R 31 -t 100 -d 5 -s $seed > "$OUT/REP_EC_sotap_R31_dp5_s${seed}.out" 2>&1 && touch "$OUT/.EC_$seed.done" && echo "done: EC $seed $(date +%H:%M:%S)"
done
echo "EC_COMPLETE $(date)"
