# Auto-generated sieve of Eratosthenes benchmark
# Limit: 12000
import time

def sieve(n):
    is_prime = [True] * (n + 1)
    is_prime[0] = False
    is_prime[1] = False
    p = 2
    while p * p <= n:
        if is_prime[p]:
            i = p * p
            while i <= n:
                is_prime[i] = False
                i += p
        p += 1
        
    checksum = 0
    for i in range(n + 1):
        if is_prime[i]:
            checksum += i
    return checksum

start = time.time()
checksum = sieve(12000)
end = time.time()

print(f"Checksum: {checksum}")
print(f"Execution Time: {end - start}s")
