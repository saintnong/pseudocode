# Performance Benchmark Results

System specifications and performance results comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: Linux 6.17.0-35-generic
- **CPU**: AMD Ryzen 7 5800H with Radeon Graphics
- **Date**: 2026-06-08 07:59:06 UTC
- **Runs**: 3

---

## Benchmark 1: Matrix Multiplication

- **Matrix Size**: 60x60 (216,000 inner loop operations)

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 1.6981s | 1.7102s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0278s | 0.0287s | Passed | 59.5x |
| Node.js | 0.0255s | 0.0262s | Passed | 65.2x |

---

## Benchmark 2: Sieve of Eratosthenes

- **Limit**: 12,000

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.1625s | 0.1642s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0115s | 0.0119s | Passed | 13.9x |
| Node.js | 0.0234s | 0.0237s | Passed | 6.9x |

---
