# Performance Benchmark Results

Matrix multiplication benchmark ($N \times N$) comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: Linux 6.17.0-35-generic
- **CPU**: AMD Ryzen 7 5800H with Radeon Graphics
- **Date**: 2026-06-08 02:51:12 UTC
- **Matrix Size**: 60x60 (216,000 inner loop operations)
- **Runs**: 3

## Results

| Language | Min Alg Time | Avg Alg Time | Min Process Time | Avg Process Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.2290s | 0.2340s | 0.2376s | 0.2424s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0086s | 0.0087s | 0.0294s | 0.0298s | Passed | 26.8x |
| Node.js | 0.0027s | 0.0029s | 0.0275s | 0.0280s | Passed | 80.6x |

*Note: Alg Time measures pure execution of the matrix multiplication algorithm, whereas Process Time includes process startup, bytecode compilation, and AST parsing.*
