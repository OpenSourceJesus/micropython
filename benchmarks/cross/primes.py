"""Prime-counting benchmark (integer, nested loop), shared by all backends.

Counts the primes below `limit` (read from the command line and converted to an
integer) by trial division up to sqrt(n). The inner loop counts divisors with an
integer accumulator (rather than a boolean flag) so it lowers identically under
every backend, including ShivyC's own compiler. Pure integer arithmetic, all
intermediates below 2**31, so every backend agrees.
"""
import sys


def count_primes(limit: int) -> int:
    count = 0
    n = 2
    while n < limit:
        divisors = 0
        d = 2
        while d * d <= n:
            if n % d == 0:
                divisors = divisors + 1
            d = d + 1
        if divisors == 0:
            count = count + 1
        n = n + 1
    return count


def main() -> int:
    limit = int(sys.argv[1])
    result = count_primes(limit)
    print(result)
    return result % 256


if __name__ == "__main__":
    sys.exit(main())
