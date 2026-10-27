#!/bin/zsh
# resumable chunk runner (scan_manifest pattern from prng-forensics)
OUT="$(cd "$(dirname "$0")" && pwd)"
cd "$OUT/../../simulators"
grep -v '^#' "$OUT/scan_manifest.txt" | while IFS='|' read -r tag seed cmd; do
  [ -z "$tag" ] && continue
  f="$OUT/REP_${tag}_s${seed}.out"
  [ -f "$OUT/.${tag}_${seed}.done" ] && continue
  eval "$cmd" > "$f" 2>&1 && touch "$OUT/.${tag}_${seed}.done" && echo "done: $tag $seed $(date +%H:%M:%S)"
done
echo "ALL_CHUNKS_COMPLETE $(date)"
