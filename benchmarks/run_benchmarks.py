#!/usr/bin/env python3
import os
import sys
import json
import time
import platform
import subprocess
import argparse
import re
import random

def get_cpu_info():
    system = platform.system()
    if system == "Linux":
        try:
            with open("/proc/cpuinfo", "r") as f:
                for line in f:
                    if "model name" in line:
                        return re.sub(r".*model name.*:\s*", "", line).strip()
        except Exception:
            pass
    elif system == "Darwin":
        try:
            return subprocess.check_output(["sysctl", "-n", "machdep.cpu.brand_string"]).decode().strip()
        except Exception:
            pass
    elif system == "Windows":
        try:
            import winreg
            key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DESCRIPTION\System\CentralProcessor\0")
            val, _ = winreg.QueryValueEx(key, "ProcessorNameString")
            return val.strip()
        except Exception:
            pass
    return platform.processor() or "Unknown CPU"

def find_scsa_binary(user_path=None):
    if user_path:
        if os.path.exists(user_path):
            return os.path.abspath(user_path)
        else:
            raise FileNotFoundError(f"Specified scsa binary not found at {user_path}")
            
    # Search common paths relative to repository root (parent of benchmarks folder)
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    possible_paths = [
        os.path.join(base_dir, "build", "scsa"),
        os.path.join(base_dir, "build", "Release", "scsa"),
        os.path.join(base_dir, "build", "Release", "scsa.exe"),
        os.path.join(base_dir, "scsa"),
        os.path.join(base_dir, "scsa.exe")
    ]
    for p in possible_paths:
        if os.path.exists(p) and os.path.isfile(p):
            if platform.system() != "Windows" and not os.access(p, os.X_OK):
                continue
            return p
            
    raise FileNotFoundError("Could not find 'scsa' binary. Please build the interpreter first.")

def generate_matrix(size):
    return [[random.randint(1, 10) for _ in range(size)] for _ in range(size)]

def multiply_matrices(A, B):
    n = len(A)
    C = [[0] * n for _ in range(n)]
    for i in range(n):
        for j in range(n):
            s = 0
            for k in range(n):
                s += A[i][k] * B[k][j]
            C[i][j] = s
    return C

def compute_checksum(C):
    return sum(sum(row) for row in C)

def parse_output(stdout):
    checksum = None
    exec_time = None
    
    # Parse Checksum: <number>
    checksum_match = re.search(r"Checksum:\s*(-?\d+)", stdout)
    if checksum_match:
        checksum = int(checksum_match.group(1))
        
    # Parse Execution Time: <number>s
    time_match = re.search(r"Execution Time:\s*([0-9.]+)\s*s", stdout)
    if time_match:
        exec_time = float(time_match.group(1))
        
    return checksum, exec_time

def main():
    parser = argparse.ArgumentParser(description="Run SCSA matrix multiplication benchmarks.")
    parser.add_argument("-s", "--size", type=int, default=60, help="Dimension N of NxN matrices (default: 60)")
    parser.add_argument("-r", "--runs", type=int, default=3, help="Number of runs to average (default: 3)")
    parser.add_argument("-p", "--scsa-path", type=str, default=None, help="Path to scsa interpreter binary")
    parser.add_argument("--skip-js", action="store_true", help="Skip Node.js benchmarking")
    args = parser.parse_args()

    benchmarks_dir = os.path.dirname(os.path.abspath(__file__))
    
    print(f"=== SCSA Benchmark Runner ===")
    print(f"Matrix Size: {args.size}x{args.size}")
    print(f"Runs: {args.runs}")
    
    # Find scsa
    try:
        scsa_bin = find_scsa_binary(args.scsa_path)
        print(f"Using SCSA binary: {scsa_bin}")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
        
    # Check if node is available
    run_js = not args.skip_js
    if run_js:
        try:
            subprocess.run(["node", "--version"], capture_output=True, check=True)
            print("Using Node.js: Available")
        except Exception:
            print("Node.js: Not available (skipping JS benchmarks)")
            run_js = False
            
    # Generate datasets
    print("Generating random matrices A and B...")
    A = generate_matrix(args.size)
    B = generate_matrix(args.size)
    
    print("Calculating expected checksum in Python...")
    expected_C = multiply_matrices(A, B)
    expected_checksum = compute_checksum(expected_C)
    print(f"Expected Checksum: {expected_checksum}")
    
    # Serialize matrices to JSON strings
    a_str = json.dumps(A)
    b_str = json.dumps(B)
    
    # Create target benchmark files
    scsa_file = os.path.join(benchmarks_dir, "matrix_multiply.scsa")
    py_file = os.path.join(benchmarks_dir, "matrix_multiply.py")
    js_file = os.path.join(benchmarks_dir, "matrix_multiply.js")
    
    # SCSA
    scsa_code = f"""# Auto-generated matrix multiplication benchmark
# Matrix size: {args.size}x{args.size}

A = {a_str}
B = {b_str}

FUNCTION matrix_multiply(A, B)
    n = A.length
    C = []
    FOR i = 0 TO n - 1
        row = []
        FOR j = 0 TO n - 1
            row.append(0)
        END FOR
        C.append(row)
    END FOR
    
    FOR i = 0 TO n - 1
        FOR j = 0 TO n - 1
            s = 0
            FOR k = 0 TO n - 1
                s = s + A[i][k] * B[k][j]
            END FOR
            C[i][j] = s
        END FOR
    END FOR
    
    RETURN C
END matrix_multiply

start = TIME()
C = matrix_multiply(A, B)
end = TIME()

n = A.length
checksum = 0
FOR i = 0 TO n - 1
    FOR j = 0 TO n - 1
        checksum = checksum + C[i][j]
    END FOR
END FOR

PRINT("Checksum: " + STRING(checksum))
PRINT("Execution Time: " + STRING(end - start) + "s")
"""
    with open(scsa_file, "w") as f:
        f.write(scsa_code)
        
    # Python
    py_code = f"""# Auto-generated matrix multiplication benchmark
# Matrix size: {args.size}x{args.size}
import time

A = {a_str}
B = {b_str}

def matrix_multiply(A, B):
    n = len(A)
    C = [[0] * n for _ in range(n)]
    for i in range(n):
        for j in range(n):
            s = 0
            for k in range(n):
                s += A[i][k] * B[k][j]
            C[i][j] = s
    return C

start = time.time()
C = matrix_multiply(A, B)
end = time.time()

n = len(A)
checksum = 0
for i in range(n):
    for j in range(n):
        checksum += C[i][j]

print(f"Checksum: {{checksum}}")
print(f"Execution Time: {{end - start}}s")
"""
    with open(py_file, "w") as f:
        f.write(py_code)
        
    # JavaScript
    js_code = f"""// Auto-generated matrix multiplication benchmark
// Matrix size: {args.size}x{args.size}
const {{ performance }} = require('perf_hooks');

const A = {a_str};
const B = {b_str};

function matrixMultiply(A, B) {{
    const n = A.length;
    const C = Array.from({{ length: n }}, () => Array(n).fill(0));
    for (let i = 0; i < n; i++) {{
        for (let j = 0; j < n; j++) {{
            let s = 0;
            for (let k = 0; k < n; k++) {{
                s += A[i][k] * B[k][j];
            }}
            C[i][j] = s;
        }}
    }}
    return C;
}}

const start = performance.now();
const C = matrixMultiply(A, B);
const end = performance.now();

const n = A.length;
let checksum = 0;
for (let i = 0; i < n; i++) {{
    for (let j = 0; j < n; j++) {{
        checksum += C[i][j];
    }}
}}

console.log("Checksum: " + checksum);
console.log("Execution Time: " + ((end - start) / 1000) + "s");
"""
    with open(js_file, "w") as f:
        f.write(js_code)
        
    print(f"Generated benchmark source files in {benchmarks_dir}.")
    
    results = {}
    
    # Runner configurations
    runs_to_do = [
        ("SCSA", [scsa_bin, scsa_file]),
        ("Python", [sys.executable, py_file])
    ]
    if run_js:
        runs_to_do.append(("NodeJS", ["node", js_file]))
        
    for name, cmd in runs_to_do:
        print(f"\nRunning {name} benchmark...")
        alg_times = []
        proc_times = []
        checksum_ok = True
        
        for r in range(args.runs):
            t0 = time.perf_counter()
            res = subprocess.run(cmd, capture_output=True, text=True)
            t1 = time.perf_counter()
            
            if res.returncode != 0:
                print(f"  Run {r+1} failed with return code {res.returncode}")
                print(f"  stderr: {res.stderr}")
                checksum_ok = False
                continue
                
            checksum, alg_time = parse_output(res.stdout)
            
            if checksum != expected_checksum:
                print(f"  Run {r+1} Checksum Mismatch! Got {checksum}, expected {expected_checksum}")
                checksum_ok = False
            else:
                proc_times.append(t1 - t0)
                if alg_time is not None:
                    alg_times.append(alg_time)
                else:
                    # fallback to process time if script didn't output time correctly
                    alg_times.append(t1 - t0)
            
            print(f"  Run {r+1}: Alg Time = {alg_time if alg_time is not None else 'N/A':.4f}s, Proc Time = {t1-t0:.4f}s")
            
        if proc_times:
            results[name] = {
                "alg_min": min(alg_times),
                "alg_avg": sum(alg_times) / len(alg_times),
                "proc_min": min(proc_times),
                "proc_avg": sum(proc_times) / len(proc_times),
                "checksum_ok": checksum_ok
            }
        else:
            results[name] = {
                "alg_min": 0,
                "alg_avg": 0,
                "proc_min": 0,
                "proc_avg": 0,
                "checksum_ok": False
            }
            
    # Compile markdown output
    cpu_info = get_cpu_info()
    os_name = platform.system()
    os_release = platform.release()
    date_time = time.strftime("%Y-%m-%d %H:%M:%S UTC", time.gmtime())
    iterations = args.size ** 3
    
    scsa_alg_min = results["SCSA"]["alg_min"]
    scsa_alg_avg = results["SCSA"]["alg_avg"]
    scsa_proc_min = results["SCSA"]["proc_min"]
    scsa_proc_avg = results["SCSA"]["proc_avg"]
    scsa_checksum = "Passed" if results["SCSA"]["checksum_ok"] else "Failed"
    
    py_alg_min = results["Python"]["alg_min"]
    py_alg_avg = results["Python"]["alg_avg"]
    py_proc_min = results["Python"]["proc_min"]
    py_proc_avg = results["Python"]["proc_avg"]
    py_checksum = "Passed" if results["Python"]["checksum_ok"] else "Failed"
    py_speedup = scsa_alg_avg / py_alg_avg if py_alg_avg > 0 else 0
    
    if "NodeJS" in results:
        js_alg_min = results["NodeJS"]["alg_min"]
        js_alg_avg = results["NodeJS"]["alg_avg"]
        js_proc_min = results["NodeJS"]["proc_min"]
        js_proc_avg = results["NodeJS"]["proc_avg"]
        js_checksum = "Passed" if results["NodeJS"]["checksum_ok"] else "Failed"
        js_speedup = scsa_alg_avg / js_alg_avg if js_alg_avg > 0 else 0
        js_row = f"| Node.js | {js_alg_min:.4f}s | {js_alg_avg:.4f}s | {js_proc_min:.4f}s | {js_proc_avg:.4f}s | {js_checksum} | {js_speedup:.1f}x |"
    else:
        js_row = "| Node.js | N/A | N/A | N/A | N/A | Skipped/Missing | N/A |"
        
    markdown_content = f"""# Performance Benchmark Results

Matrix multiplication benchmark ($N \\times N$) comparing the SCSA Pseudocode Interpreter to Python and Node.js.

## System Information
- **OS**: {os_name} {os_release}
- **CPU**: {cpu_info}
- **Date**: {date_time}
- **Matrix Size**: {args.size}x{args.size} ({iterations:,} inner loop operations)
- **Runs**: {args.runs}

## Results

| Language | Min Alg Time | Avg Alg Time | Min Process Time | Avg Process Time | Checksum | Relative Speed (vs SCSA) |
| --- | --- | --- | --- | --- | --- | --- |
| SCSA Pseudocode | {scsa_alg_min:.4f}s | {scsa_alg_avg:.4f}s | {scsa_proc_min:.4f}s | {scsa_proc_avg:.4f}s | {scsa_checksum} | 1.0x (Baseline) |
| Python 3 | {py_alg_min:.4f}s | {py_alg_avg:.4f}s | {py_proc_min:.4f}s | {py_proc_avg:.4f}s | {py_checksum} | {py_speedup:.1f}x |
{js_row}

*Note: Alg Time measures pure execution of the matrix multiplication algorithm, whereas Process Time includes process startup, bytecode compilation, and AST parsing.*
"""

    results_md_path = os.path.join(benchmarks_dir, "README.md")
    with open(results_md_path, "w") as f:
        f.write(markdown_content)
        
    print(f"\nBenchmark completed successfully! Results written to {results_md_path}")

if __name__ == "__main__":
    main()
