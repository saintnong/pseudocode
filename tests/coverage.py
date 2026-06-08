#!/usr/bin/env python3
import os
import sys
import subprocess
import re
import glob

# ANSI color codes
class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    END = '\033[0m'

def run_cmd(args, cwd=None):
    result = subprocess.run(args, capture_output=True, text=True, cwd=cwd)
    if result.returncode != 0:
        print(f"{Colors.RED}Command failed: {' '.join(args)}{Colors.END}")
        print(result.stdout)
        print(result.stderr)
        sys.exit(result.returncode)
    return result.stdout

def main():
    # Make sure we're in the repository root
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(repo_root)

    print(f"{Colors.BOLD}{Colors.CYAN}=== Code Coverage Setup ==={Colors.END}")
    
    # 1. Configure and Build with Coverage Enablement
    print("Configuring and building project with coverage instrumentations...")
    run_cmd(["cmake", "-DENABLE_COVERAGE=ON", "-B", "build"])
    run_cmd(["cmake", "--build", "build"])

    # 2. Reset existing coverage data to avoid accumulation from old runs
    print("Resetting previous coverage profiles...")
    gcda_files = glob.glob("build/**/*.gcda", recursive=True)
    for f in gcda_files:
        try:
            os.remove(f)
        except OSError:
            pass

    # 3. Run the Test Suite
    print("Running integration tests...")
    test_runner = os.path.join("tests", "tester.py")
    interpreter = os.path.join("build", "scsa")
    run_cmd(["python3", test_runner, interpreter])

    # 4. Generate Coverage stats via gcov
    print("Analyzing coverage notes with gcov...")
    src_files = glob.glob("src/*.cpp") + glob.glob("src/*.hpp")
    
    coverage_results = []
    
    for src_file in sorted(src_files):
        filename = os.path.basename(src_file)
        gcno_path = f"build/CMakeFiles/scsa.dir/src/{filename}.gcno"
        
        if not os.path.exists(gcno_path):
            continue
            
        gcov_output = run_cmd(["gcov", "-o", gcno_path, src_file])
        
        # Parse gcov output block for this specific source file
        lines = gcov_output.splitlines()
        found_file = False
        lines_executed_pct = 0.0
        total_lines = 0
        
        for i, line in enumerate(lines):
            # Check if this section belongs to our source file
            if line.startswith("File '") and src_file in line:
                found_file = True
                # The next line contains the execution percentage
                if i + 1 < len(lines):
                    match = re.search(r"Lines executed:([\d\.]+)% of (\d+)", lines[i+1])
                    if match:
                        lines_executed_pct = float(match.group(1))
                        total_lines = int(match.group(2))
                break
                
        if found_file:
            coverage_results.append({
                "file": src_file,
                "percentage": lines_executed_pct,
                "total_lines": total_lines
            })

    # Clean up generated .gcov files from the root
    for f in glob.glob("*.gcov"):
        try:
            os.remove(f)
        except OSError:
            pass

    # 5. Display Coverage Summary
    print(f"\n{Colors.BOLD}{Colors.CYAN}=== Code Coverage Summary ==={Colors.END}\n")
    print(f"{Colors.BOLD}{'SOURCE FILE':<25} {'COVERAGE':<12} {'EXECUTABLE LINES':<15}{Colors.END}")
    print("-" * 57)
    
    total_executable = 0
    total_covered = 0

    for res in coverage_results:
        color = Colors.GREEN if res["percentage"] >= 90 else (Colors.YELLOW if res["percentage"] >= 70 else Colors.RED)
        percent_str = f"{res['percentage']:.2f}%"
        print(f"{res['file']:<25} {color}{percent_str:<12}{Colors.END} {res['total_lines']:<15}")
        
        total_executable += res["total_lines"]
        total_covered += int(res["total_lines"] * (res["percentage"] / 100.0))

    print("-" * 57)
    overall_pct = (total_covered / total_executable * 100) if total_executable > 0 else 0.0
    overall_color = Colors.GREEN if overall_pct >= 90 else (Colors.YELLOW if overall_pct >= 70 else Colors.RED)
    print(f"{Colors.BOLD}{'OVERALL PROJECT':<25} {overall_color}{overall_pct:.2f}%{Colors.END} {total_executable:<15}\n")

if __name__ == "__main__":
    main()
