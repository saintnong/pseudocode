# Integration Test Suite Runner
import os
import sys
import subprocess
import pathlib
import re

# ANSI Color Codes for cleaner CLI output
class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'

def run_test(interpreter_path, file_path):
    """Executes a single .scsa file and compares output/errors to expectations."""
    with open(file_path, 'r') as f:
        content = f.read()
    
    expect_matches = re.findall(r'#\s*EXPECT:\s*(.*)', content)
    error_matches = re.findall(r'#\s*EXPECT ERROR:\s*(.*)', content)

    if not expect_matches and not error_matches:
        return "SKIP", "No expectations defined"

    is_error_test = len(error_matches) > 0
    expected_text = "\n".join(error_matches if is_error_test else expect_matches).strip()

    try:
        result = subprocess.run(
            [interpreter_path, str(file_path)],
            capture_output=True,
            text=True,
            timeout=2 
        )
        
        actual_stdout = result.stdout.strip()
        actual_stderr = result.stderr.strip()
        combined_output = f"{actual_stdout}\n{actual_stderr}".strip()

        # Filter out lines from the output that contain test harness triggers
        # This stops the tester from finding its own test cases and passing them
        combined_output = "\n".join([
            line for line in combined_output.splitlines() 
            if "# EXPECT" not in line
        ])

        if is_error_test:
            if result.returncode == 0:
                return "FAIL", "Expected non-zero exit code but got 0"
            if expected_text in combined_output:
                return "PASS", ""
            return "FAIL", f"Expected error \"{expected_text}\" not found.\nActual output:\n{combined_output}"

        else:
            if result.returncode != 0:
                return "FAIL", f"Crashed (Exit {result.returncode}): {actual_stderr}"
            if actual_stdout == expected_text:
                return "PASS", ""
            return "FAIL", f"Expected:  {expected_text.splitlines()} \n       Actual:    {actual_stdout.splitlines()} "

    except subprocess.TimeoutExpired:
        return "FAIL", "Timed out (2s limit)"
    except Exception as e:
        return "FAIL", f"Test Runner Error: {str(e)}"

def main():
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <path_to_interpreter>")
        sys.exit(1)

    interpreter = os.path.abspath(sys.argv[1])
    
    # Set working directory to the script's directory
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    root_dir = pathlib.Path('.')
    
    counts = {"PASS": 0, "FAIL": 0, "SKIP": 0}

    print(f"\n{Colors.BOLD}{Colors.CYAN}=== Integration Tests ==={Colors.END}")
    print(f"{Colors.BOLD}Interpreter:{Colors.END} {interpreter}\n")

    for suite_path in sorted(root_dir.iterdir()):
        if suite_path.is_dir() and not suite_path.name.startswith('.'):
            scsa_files = list(suite_path.glob('**/*.scsa'))
            if not scsa_files: continue

            print(f"{Colors.UNDERLINE}Suite: {suite_path.name}{Colors.END}")
            
            for test_file in sorted(scsa_files):
                status, message = run_test(interpreter, test_file)
                counts[status] += 1
                
                # Format the status label
                if status == "PASS":
                    label = f"{Colors.GREEN}PASS{Colors.END}"
                elif status == "SKIP":
                    label = f"{Colors.YELLOW}SKIP{Colors.END}"
                else:
                    label = f"{Colors.RED}FAIL{Colors.END}"

                # Print results in aligned columns
                print(f"  {label:12} {test_file.name}")
                if status == "FAIL":
                    print(f"  {Colors.RED}---> {message}{Colors.END}")
            print()

    # Final Summary Box
    total = sum(counts.values())
    print(f"{Colors.BOLD}--- Result Summary ---{Colors.END}")
    print(f" Count:    {total}")
    print(f"  {Colors.GREEN}PASS:    {counts['PASS']}{Colors.END}")
    print(f"  {Colors.RED}FAIL:    {counts['FAIL']}{Colors.END}")
    print(f"  {Colors.YELLOW}SKIP:    {counts['SKIP']}{Colors.END}")
    print("----------------------\n")

    if counts["FAIL"] > 0:
        sys.exit(1)

if __name__ == "__main__":
    main()