#!/usr/bin/env python3
"""Print a window of the minified bundle around a needle. grep -o chokes on the
1.3 MB single line; this does not."""
import sys
path, needle = sys.argv[1], sys.argv[2]
before = int(sys.argv[3]) if len(sys.argv) > 3 else 200
after = int(sys.argv[4]) if len(sys.argv) > 4 else 800
nth = int(sys.argv[5]) if len(sys.argv) > 5 else 0
s = open(path, encoding="utf-8", errors="replace").read()
i, hits = -1, 0
while True:
    i = s.find(needle, i + 1)
    if i < 0:
        print(f"-- no occurrence #{nth} of {needle!r} (found {hits})")
        break
    if hits == nth:
        print(s[max(0, i - before): i + after])
        break
    hits += 1
