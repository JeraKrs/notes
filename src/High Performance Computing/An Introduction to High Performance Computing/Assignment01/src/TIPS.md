## The Optimisation Tips

### Data Layout and Data Type

* Column Major Order -> Row Major Order
* Double -> Float


### Loop Fusion

* Removed `if (row != col)` statement
* Using multiple operation to replace division and square root


### Compiler and Flags

* gcc -> icc
* adding `-O3` flag in using gcc compiler, `-fast` flag in using icc compiler


### SIMD

* simd
* avx


### Algorithm

* Gaussian Elimination
