# Performance Benchmark Results

System specifications and performance results comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: Linux 6.17.0-35-generic
- **CPU**: AMD Ryzen 7 5800H with Radeon Graphics
- **Date**: 2026-06-08 06:16:47 UTC
- **Runs**: 3

---

## Benchmark 1: Matrix Multiplication

- **Matrix Size**: 60x60 (216,000 inner loop operations)

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0713s | 0.0724s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0281s | 0.0292s | Passed | 2.5x |
| Node.js | 0.0261s | 0.0266s | Passed | 2.7x |

---

## Benchmark 2: Sieve of Eratosthenes

- **Limit**: 12,000

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0136s | 0.0140s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0108s | 0.0110s | Passed | 1.3x |
| Node.js | 0.0220s | 0.0231s | Passed | 0.6x |

---
