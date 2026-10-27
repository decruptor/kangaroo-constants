#!/bin/zsh
# Long-cycle occupancy + solve-channel battery (third-referee items 3 and 4).
# Sequential, nice'd; writes one .out per config into ../out/.
cd "$(dirname "$0")"
S=./kangsim_diag
O=../out
run() { tag=$1; shift; if [ -s "$O/cyclog2_$tag.out" ]; then echo "skip $tag"; return; fi
  echo "=== $tag $(date -u +%H:%M:%S)"; nice -n 10 $S "$@" > "$O/cyclog2_$tag.out" 2>&1
  tail -4 "$O/cyclog2_$tag.out" | head -3; }
# solve channel (wild-wild share of SOLVES)
run ww_n32_t1500 -m 3 -n 32 -t 1500 -d 5
run ww_n41_t300  -m 3 -n 41 -t 300 -d 5
# long-cycle lives at dp14 (simple handling, h=512): expect ~6.6% of lives in long cycle
run simple_dp14_h512 -m 3 -n 48 -t 60 -d 14 -cyclog2
# escape rule h=512: expect ~8.5% trapped by 26k steps, ~4% of steps wasted
run esc_dp14_h512    -m 3 -n 48 -t 200 -d 14 -esc -cyclog2
# escape rule h=2048: expect ~0
run esc_dp14_h2048   -m 3 -n 48 -t 60 -k 2048 -d 14 -esc -cyclog2
# dp20 / dp24 simple: occupancy trajectory (herd stall)
run simple_dp20_h512 -m 3 -n 48 -t 60 -d 20 -cyclog2
run simple_dp24_h512 -m 3 -n 48 -t 20 -d 24 -cyclog2
echo "BATTERY DONE $(date -u +%H:%M:%S)"
