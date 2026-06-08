# Performance Benchmark Results

Matrix multiplication benchmark ($N \times N$) comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: Linux 6.17.0-35-generic
- **CPU**: AMD Ryzen 7 5800H with Radeon Graphics
- **Date**: 2026-06-08 04:23:41 UTC
- **Matrix Size**: 60x60 (216,000 inner loop operations)
- **Runs**: 3

## Results

| Language | Min Alg Time | Avg Alg Time | Min Process Time | Avg Process Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0600s | 0.0607s | 0.0745s | 0.0749s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0085s | 0.0087s | 0.0283s | 0.0287s | Passed | 7.0x |
| Node.js | 0.0026s | 0.0027s | 0.0254s | 0.0274s | Passed | 22.2x |

*Note: Alg Time measures pure execution of the matrix multiplication algorithm, whereas Process Time includes process startup, bytecode compilation, and AST parsing.*
