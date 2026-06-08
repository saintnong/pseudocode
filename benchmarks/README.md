# Performance Benchmark Results

System specifications and performance results comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: Linux 6.17.0-35-generic
- **CPU**: AMD Ryzen 7 5800H with Radeon Graphics
- **Date**: 2026-06-08 07:55:30 UTC
- **Runs**: 3

---

## Benchmark 1: Matrix Multiplication

- **Matrix Size**: 60x60 (216,000 inner loop operations)

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 1.8251s | 1.8293s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0283s | 0.0286s | Passed | 64.0x |
| Node.js | 0.0252s | 0.0254s | Passed | 72.0x |

---

## Benchmark 2: Sieve of Eratosthenes

- **Limit**: 12,000

| Language | Min Time | Avg Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- |
| SCSA Pseudocode | 0.1725s | 0.1747s | Passed | 1.0x (Baseline) |
| Python 3 | 0.0105s | 0.0108s | Passed | 16.1x |
| Node.js | 0.0224s | 0.0230s | Passed | 7.6x |

---
