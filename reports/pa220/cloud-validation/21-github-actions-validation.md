# PA-220 GitHub Actions validation

- Branch: feature/pa220-ubootkit-web
- Commit: 62e2a0b7b96f5b0b0dbca9628fe451a456472193
- TCP source SHA-1: 8dae726f8dccb2ebdbaee949455d8131dcd4c80d
- TCP header SHA-1: db4752ce1c9589c9579069e40476279c0eabda75
- Host regression source: reports/pa220/P5-1A.3/18-host-focused-protocol-retest.c
- Host compile exit code: 0
- Host test exit code: 0
- Private SDK release download exit code: 0
- SDK archive checksum exit code: 0
- OCTEON build exit code: 0

The full command output is in [21-github-actions-validation.log](21-github-actions-validation.log).

- Host assertions: 76 passed, 0 failed
- CLOSE-WAIT post-FIN payload: rejected
- TIME-WAIT duplicate FIN: ACK emitted; 60-second deadline preserved
- OCTEON output: `bin/pa220/u-boot-octeon_pa220.bin`, 1,362,216 bytes
- RAM boot / physical TCP network test: NOT RUN
