# P5-2A build and commit record

- Repository: sdzbzyh/uboot-qsdk12.5-build
- Branch: feature/pa220-ubootkit-web
- P5-2A validation input commit: 3ff73d0627bcc7007e9f1d6ba8e705cc895330da
- GitHub Actions run: 44887007342
- Workflow result: success (Host regression + OCTEON target object compile + N32 relocatable link)
- PA-220 source checkout: IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- Host test source: host-test.c (maintained at tests/host/http_request/test.c)
- Parser source/header: http_request.c and http_request.h (maintained under u-boot/)
- OCTEON parser object SHA-256: ae6d2491f5eea61bf70051ecd9bca9da5e91275e79020eae1f694592cdd0a008
- OCTEON relocatable object SHA-256: 5ce690b203c403a8895cb6fcfb09bb4e9a74f7fc3b685bd28e079fc25b31ebc0
- Final archive commit: recorded in the feature branch history; see final completion response for its exact SHA.
- Earlier failed CI attempts are retained in Git history; only run 44887007342 is the final passing validation.
