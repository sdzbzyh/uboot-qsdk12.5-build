# P5-2A build and commit record

- Repository: sdzbzyh/uboot-qsdk12.5-build
- Branch: feature/pa220-ubootkit-web
- P5-2A validation input commit: 5a02e0b7701cedab6e355f47d55f8f59679a2221
- GitHub Actions run: 44886769145
- Workflow result: success (Host regression + OCTEON target object compile + N32 relocatable link)
- PA-220 source checkout: IronDemiGod/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- Host test source: host-test.c (same source is maintained at tests/host/http_request/test.c)
- Parser source/header: http_request.c and http_request.h (same sources are maintained under u-boot/)
- OCTEON parser object SHA-256: 9d0609254d6dd83c269d96080e0391b581d192693d16861039b807dd936b4d94
- OCTEON relocatable object SHA-256: 7e569b5a4183835617b75d0406a7cfd19cc02df3d02ae31b605a6936ad020156
- Final report files are committed to this branch; the containing commit is recorded by the Git history and the final P5-2A completion response.
- Earlier failed CI attempts were retained in Git history; only run 44886769145 is the final passing validation.
