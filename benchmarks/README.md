# Performance Benchmark Results

System specifications and performance results comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: Linux 6.17.0-35-generic
- **CPU**: AMD Ryzen 7 5800H with Radeon Graphics
- **Date**: 2026-06-08 08:01:42 UTC
- **Runs**: 3

---

## Benchmark 1: Matrix Multiplication

- **Matrix Size**: 60x60 (216,000 inner loop operations)

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 1.7248s | 1.7463s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0304s | 0.0306s | Passed | 57.1x |
| Node.js | 0.0264s | 0.0276s | Passed | 63.3x |

---

## Benchmark 2: Sieve of Eratosthenes

- **Limit**: 12,000

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.1627s | 0.1661s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0119s | 0.0129s | Passed | 12.8x |
| Node.js | 0.0254s | 0.0270s | Passed | 6.1x |

---
