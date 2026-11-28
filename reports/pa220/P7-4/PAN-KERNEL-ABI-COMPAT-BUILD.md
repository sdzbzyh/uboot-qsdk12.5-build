# PA-220 PAN kernel bootinfo ABI compatibility build

- Branch: feature/pa220-ubootkit-web
- Source baseline: sdzbzyh/pa220-openwrt@801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8
- Integration commit: 478597e5bf4c6af1a77cbae75db708b330331e70
- SDK release: octeon-sdk-5.1.0
- SDK download exit code: 0
- SDK checksum exit code: 0
- U-Boot build exit code: 0
- Image size: 1402792 bytes
- Image SHA-256: 2f0fa866999c6684e0f32261da11472927321e4c087eceb862ff6621f3edbc11

The build serializes the PAN-OS 3.10.87 bootinfo copy with its 24-byte
extension at offsets 152..175, FDT address at 176, and extended coremask
at 184. The coremask is synthesized from the validated legacy mask.

Full output: PAN-KERNEL-ABI-COMPAT-BUILD.log
