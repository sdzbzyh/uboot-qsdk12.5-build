# PA-220 OEM Stage-3 and rebuilt U-Boot FDT comparison

Read-only comparison of the original SPI Stage-3 FDT and the FDT embedded in the rebuilt PA-220 U-Boot image.

| Field | OEM SPI Stage 3 | Rebuilt U-Boot |
|---|---:|---:|
| FDT offset | 0x1c9460 | 0x113910 |
| FDT size | 10850 | 10277 |
| Node count | 59 | 52 |
| Model | pan,pa-220 | pan,pa-220 |
| Compatible | cavium,tb7100 | pan,pa-220 | cavium,tb7100 |

## MMC controller

OEM: [{"path": "/soc@0/mmc@1180000002000", "compatible": "cavium,octeon-6130-mmc", "reg": "cells:00011800 00002000 00000000 00000100 00011800 00000168 00000000 00000020", "status": "ABSENT"}, {"path": "/soc@0/mmc@1180000002000/mmc-slot@0", "compatible": "cavium,octeon-6130-mmc-slot | mmc-spi-slot", "reg": "cells:00000000", "status": "ABSENT"}]
Rebuilt: [{"path": "/soc@0/mmc@1180000002000", "compatible": "cavium,octeon-6130-mmc", "reg": "cells:00011800 00002000 00000000 00000100 00011800 00000168 00000000 00000020", "status": "ABSENT"}, {"path": "/soc@0/mmc@1180000002000/mmc-slot@0", "compatible": "cavium,octeon-6130-mmc-slot | mmc-spi-slot", "reg": "cells:00000000", "status": "ABSENT"}]

## CN7130 USB controller

OEM: [{"path": "/soc@0/interrupt-controller@107000000e800", "compatible": "cavium,octeon-7130-cib", "reg": "cells:00010700 0000e800 00000000 00000008 00010700 0000ea00 00000000 00000008", "status": "ABSENT"}, {"path": "/soc@0/interrupt-controller@107000000e900", "compatible": "cavium,octeon-7130-cib", "reg": "cells:00010700 0000e900 00000000 00000008 00010700 0000eb00 00000000 00000008", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180068000000", "compatible": "cavium,octeon-7130-usb-uctl", "reg": "cells:00011800 68000000 00000000 00000100", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180068000000/xhci@1680000000000", "compatible": "cavium,octeon-7130-xhci | synopsys,dwc3", "reg": "cells:00016800 00000000 00000010 00000000", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180069000000", "compatible": "cavium,octeon-7130-usb-uctl", "reg": "cells:00011800 69000000 00000000 00000100", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180069000000/xhci@1690000000000", "compatible": "cavium,octeon-7130-xhci | synopsys,dwc3", "reg": "cells:00016900 00000000 00000010 00000000", "status": "ABSENT"}]
Rebuilt: [{"path": "/soc@0/interrupt-controller@107000000e800", "compatible": "cavium,octeon-7130-cib", "reg": "cells:00010700 0000e800 00000000 00000008 00010700 0000ea00 00000000 00000008", "status": "ABSENT"}, {"path": "/soc@0/interrupt-controller@107000000e900", "compatible": "cavium,octeon-7130-cib", "reg": "cells:00010700 0000e900 00000000 00000008 00010700 0000eb00 00000000 00000008", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180068000000", "compatible": "cavium,octeon-7130-usb-uctl", "reg": "cells:00011800 68000000 00000000 00000100", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180068000000/xhci@1680000000000", "compatible": "cavium,octeon-7130-xhci | synopsys,dwc3", "reg": "cells:00016800 00000000 00000010 00000000", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180069000000", "compatible": "cavium,octeon-7130-usb-uctl", "reg": "cells:00011800 69000000 00000000 00000100", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180069000000/xhci@1690000000000", "compatible": "cavium,octeon-7130-xhci | synopsys,dwc3", "reg": "cells:00016900 00000000 00000010 00000000", "status": "ABSENT"}]

## Structural differences

Nodes only in rebuilt: /memory, /soc@0/i2c@1180000001000/temp-sensor@4c
Nodes only in OEM: /gpio-leds, /soc@0/i2c@1180000001000/fan-ctlr@2c, /soc@0/i2c@1180000001000/tlv-eeprom@56, /soc@0/i2c@1180000001000/tmp@48, /soc@0/i2c@1180000001000/tmp@49, /soc@0/i2c@1180000001000/tmp@4a, /soc@0/i2c@1180000001000/tmp@4b, /soc@0/i2c@1180000001000/tmp@4c, /soc@0/i2c@1180000001000/volt-monitor@35
Critical property differences: 4
- / compatible: OEM=cavium,tb7100; rebuilt=pan,pa-220 | cavium,tb7100
- /soc@0/mmc@1180000002000 power-gpios: OEM=cells:00000002 00000008 00000000; rebuilt=cells:0000000b 00000008 00000000
- /soc@0/uctl@1180068000000 power: OEM=cells:00000002 00000002 00000000; rebuilt=cells:0000000b 00000002 00000000
- /soc@0/uctl@1180069000000 power: OEM=cells:00000002 00000003 00000000; rebuilt=cells:0000000b 00000003 00000000

## Toolchain bootinfo ABI

- external/pa220-openwrt/u-boot/arch/mips/include/asm/arch-octeon/cvmx-app-init.h: CVMX_BOOTINFO_MIN_VER=4

## Limits

- FDT similarity alone does not prove Linux accepted the passed FDT.
- The live kernel message 'Using internal Device Tree.' is separate evidence of fallback selection.
- No device access or SPI/eMMC writes are performed.

Full machine-readable inventory: boot-handoff-audit.json
