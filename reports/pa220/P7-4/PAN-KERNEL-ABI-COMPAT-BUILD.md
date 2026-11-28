# PA-220 PAN kernel bootinfo ABI compatibility build

- Branch: feature/pa220-ubootkit-web
- Source baseline: sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- Integration commit: 509db0014feaec7db4bfd3efb1ec4c38283d739c
- SDK release: octeon-sdk-5.1.0
- SDK download exit code: 0
- SDK checksum exit code: 0
- U-Boot build exit code: 0
- Image size: 1402792 bytes
- Image SHA-256: 318c319b824603ee3080ab12687e940ec317c9216566b1986ea8a9bde9048d1d

The build serializes the PAN-OS 3.10.87 bootinfo copy with its 24-byte
extension at offsets 152..175, FDT address at 176, and extended coremask
at 184. The coremask is synthesized from the validated legacy mask.

Full output: PAN-KERNEL-ABI-COMPAT-BUILD.log
