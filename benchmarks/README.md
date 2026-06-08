# Performance Benchmark Results

System specifications and performance results comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: Linux 6.17.0-35-generic
- **CPU**: AMD Ryzen 7 5800H with Radeon Graphics
- **Date**: 2026-06-08 06:45:57 UTC
- **Runs**: 3

---

## Benchmark 1: Matrix Multiplication

- **Matrix Size**: 60x60 (216,000 inner loop operations)

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0725s | 0.0726s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0294s | 0.0311s | Passed | 2.3x |
| Node.js | 0.0265s | 0.0270s | Passed | 2.7x |

---

## Benchmark 2: Sieve of Eratosthenes

- **Limit**: 12,000

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0142s | 0.0143s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0113s | 0.0116s | Passed | 1.2x |
| Node.js | 0.0224s | 0.0236s | Passed | 0.6x |

---
