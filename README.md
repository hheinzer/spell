# spell

C99 implementation of [Peter Norvigs spelling corrector](https://norvig.com/spell-correct.html).

Input files can be downloaded here:
- [big.txt](https://norvig.com/big.txt)
- [spell-testset1.txt](https://norvig.com/spell-testset1.txt)
- [spell-testset2.txt](https://norvig.com/spell-testset2.txt)

## Performance

On my machine, using `hyperfine` I got:
```
$ hyperfine --warmup 3 "./spell_O1" "./spell_O2" "./spell_O3" "./spell_Ofast" "./spell_O2_fast-math"
Benchmark 1: ./spell_O1
  Time (mean ± σ):      2.461 s ±  0.029 s    [User: 2.442 s, System: 0.016 s]
  Range (min … max):    2.426 s …  2.518 s    10 runs

Benchmark 2: ./spell_O2
  Time (mean ± σ):      2.429 s ±  0.035 s    [User: 2.408 s, System: 0.017 s]
  Range (min … max):    2.385 s …  2.497 s    10 runs

Benchmark 3: ./spell_O3
  Time (mean ± σ):      3.016 s ±  0.032 s    [User: 2.992 s, System: 0.019 s]
  Range (min … max):    2.949 s …  3.058 s    10 runs

Benchmark 4: ./spell_Ofast
  Time (mean ± σ):      2.982 s ±  0.009 s    [User: 2.956 s, System: 0.021 s]
  Range (min … max):    2.967 s …  2.994 s    10 runs

Benchmark 5: ./spell_O2_fast-math
  Time (mean ± σ):      2.938 s ±  0.006 s    [User: 2.918 s, System: 0.017 s]
  Range (min … max):    2.929 s …  2.947 s    10 runs

Summary
  './spell_O2' ran
    1.01 ± 0.02 times faster than './spell_O1'
    1.21 ± 0.02 times faster than './spell_O2_fast-math'
    1.23 ± 0.02 times faster than './spell_Ofast'
    1.24 ± 0.02 times faster than './spell_O3'
```
That is why I use `-O2`.
