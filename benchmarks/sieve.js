// Auto-generated sieve of Eratosthenes benchmark
// Limit: 12000
const { performance } = require('perf_hooks');

function sieve(n) {
    const is_prime = Array(n + 1).fill(true);
    is_prime[0] = false;
    is_prime[1] = false;
    let p = 2;
    while (p * p <= n) {
        if (is_prime[p]) {
            let i = p * p;
            while (i <= n) {
                is_prime[i] = false;
                i += p;
            }
        }
        p += 1;
    }
    let checksum = 0;
    for (let i = 0; i <= n; i++) {
        if (is_prime[i]) {
            checksum += i;
        }
    }
    return checksum;
}

const start = performance.now();
const checksum = sieve(12000);
const end = performance.now();

console.log("Checksum: " + checksum);
console.log("Execution Time: " + ((end - start) / 1000) + "s");
