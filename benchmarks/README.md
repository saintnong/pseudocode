# Performance Benchmark Results

System specifications and performance results comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: Linux 6.17.0-35-generic
- **CPU**: AMD Ryzen 7 5800H with Radeon Graphics
- **Date**: 2026-06-08 04:43:23 UTC
- **Runs**: 3

---

## Benchmark 1: Matrix Multiplication

Matrix multiplication ($N \times N$) benchmark measuring nested loops and element-wise array operations.

- **Matrix Size**: 60x60 (216,000 inner loop operations)

| Language | Min Alg Time | Avg Alg Time | Min Process Time | Avg Process Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0630s | 0.0673s | 0.0759s | 0.0811s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0085s | 0.0087s | 0.0283s | 0.0287s | Passed | 7.7x |
| Node.js | 0.0027s | 0.0027s | 0.0261s | 0.0271s | Passed | 25.1x |

---

## Benchmark 2: Sieve of Eratosthenes

Prime number generation using the Sieve of Eratosthenes, measuring array allocation, indexing, dynamic resizing/multiplication, and sequential scans.

- **Limit**: 12,000

| Language | Min Alg Time | Avg Alg Time | Min Process Time | Avg Process Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0050s | 0.0057s | 0.0135s | 0.0137s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0008s | 0.0009s | 0.0111s | 0.0112s | Passed | 6.2x |
| Node.js | 0.0007s | 0.0007s | 0.0233s | 0.0239s | Passed | 7.8x |

---

*Note: Alg Time measures pure execution of the algorithm, whereas Process Time includes process startup, bytecode compilation, and AST parsing.*
