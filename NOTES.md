Cache Lab — Working Notes
=========================

Hand-written build, run, and grading notes for this Cache Lab tree.
The CS:APP `README` is the upstream handout and is left untouched;
everything written by hand lives here.


Environment
-----------

This host is Apple Silicon (arm64). The grading Makefile compiles with
`-m64` (x86-64), which does not run natively here, so the work is split:

- Edit and build locally on the Mac (arm64).
- Grade inside an x86-64 docker container (`./grade.sh`).

The prebuilt binaries `csim-ref`, `test-csim`, and `test-trans` are 2016
x86 executables and abort with `exec format error` on the Mac. Use them
only under docker, at grading time.


Building
--------

Local study build (arm64; omits the grading-only `-m64`):

```
clang -g -Wall -std=c99 csim.c cachelab.c -o csim_local -lm && ./csim_local
```

```
-g          emit debug information
-Wall       enable all warnings
-std=c99    compile to the C99 standard
-lm         link the math library
```

`&&` runs the binary only if the build succeeds, so a broken build never
executes a stale binary. `csim_local` is a throwaway study binary and is
git-ignored.


Grading
-------

Full grade under docker (x86-64):

```
./grade.sh
```


Progress
--------

Part A -- `csim.c` (cache simulator):

- [x] A-1  build / run / verify loop
- [ ] A-2  parse CLI args (-h -v -s -E -b -t)
- [ ] A-3  read the trace file line by line
- [ ] A-4  ignore instruction loads (`I`)
- [ ] A-5  cache data structure (set x line; valid / tag / lru)
- [ ] A-6  address split (set index / tag)
- [ ] A-7  access decision: hit / miss / eviction with LRU
- [ ] A-8  handle L / S / M ops
           (`M` = load followed by store, so miss+hit or hit+hit)
- [ ] A-9  printSummary(hit_count, miss_count, eviction_count)
- [ ] A-10 malloc / free cleanup
- [ ] A-11 pass `test-csim`

Part B -- `trans.c` (cache-optimized matrix transpose):

Target for full credit: 32x32 miss < 300, 64x64 miss < 1300,
61x67 miss < 2000.

- [ ] B-1  explain why naive transpose misses
- [ ] B-2  preserve transpose semantics: `B[j][i] = A[i][j]`
- [ ] B-3  respect lab constraints
           (no arrays/malloc/recursion; limited local ints; do not modify A)
- [ ] B-4  32x32 blocking
- [ ] B-5  64x64 blocking with conflict-miss handling
- [ ] B-6  61x67 irregular blocking
- [ ] B-7  test each case with `test-trans`
- [ ] B-8  run full grade through `./grade.sh`


Notes
-----

Filled in as friction is hit and resolved.
