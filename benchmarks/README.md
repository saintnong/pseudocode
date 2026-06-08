# Performance Benchmark Results

System specifications and performance results comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: Linux 6.17.0-35-generic
- **CPU**: AMD Ryzen 7 5800H with Radeon Graphics
- **Date**: 2026-06-08 05:01:34 UTC
- **Runs**: 3

---

## Benchmark 1: Matrix Multiplication

- **Matrix Size**: 60x60 (216,000 inner loop operations)

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0728s | 0.0739s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0286s | 0.0289s | Passed | 2.6x |
| Node.js | 0.0257s | 0.0269s | Passed | 2.8x |

---

## Benchmark 2: Sieve of Eratosthenes

- **Limit**: 12,000

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0138s | 0.0147s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0112s | 0.0113s | Passed | 1.3x |
| Node.js | 0.0239s | 0.0249s | Passed | 0.6x |

---
