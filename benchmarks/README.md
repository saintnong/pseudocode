# Performance Benchmark Results

System specifications and performance results comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: Linux 6.17.0-35-generic
- **CPU**: AMD Ryzen 7 5800H with Radeon Graphics
- **Date**: 2026-06-08 05:46:20 UTC
- **Runs**: 3

---

## Benchmark 1: Matrix Multiplication

- **Matrix Size**: 60x60 (216,000 inner loop operations)

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0726s | 0.0746s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0285s | 0.0285s | Passed | 2.6x |
| Node.js | 0.0250s | 0.0674s | Passed | 1.1x |

---

## Benchmark 2: Sieve of Eratosthenes

- **Limit**: 12,000

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.0131s | 0.0134s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0103s | 0.0106s | Passed | 1.3x |
| Node.js | 0.0221s | 0.0240s | Passed | 0.6x |

---
