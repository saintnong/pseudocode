# Full clone of examples/bench.scsa in Python

import time

def check_primes(limit):
    count = 0
    num = 2
    
    while num <= limit:
        is_prime = True
        divisor = 2
        
        check_limit = num // 2
        
        while divisor <= check_limit:
            test_val = num
            while test_val >= divisor:
                test_val = test_val - divisor
            
            if test_val == 0:
                is_prime = False
                divisor = check_limit 
            
            divisor = divisor + 1
            
        if is_prime == True:
            count = count + 1
            
        num = num + 1
        
    return count

print("Target: Find all primes up to 2000")

start = time.time()
total_found = check_primes(2000)
end = time.time()

total_time = end - start

print("Primes found: " + str(total_found))
print("Execution Time: " + str(round(total_time, 4)) + "s")