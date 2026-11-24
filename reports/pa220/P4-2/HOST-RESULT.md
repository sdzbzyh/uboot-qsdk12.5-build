# P4-2 Host Harness Result

Production source: u-boot/arch/mips/cpu/octeon/octeon_load_range.c

    HOST RAM RANGE: cases=32 assertions=121 failures=0 writes=1

- GCC C99, -Wall -Wextra -Werror: PASS
- AddressSanitizer + UndefinedBehaviorSanitizer: PASS
- All rejected-range guarded-write checks: no write, destination unchanged.
- The final writes=1 is the accepted write from the final positive KSEG1 normalization case; every rejection kept the counter unchanged.
