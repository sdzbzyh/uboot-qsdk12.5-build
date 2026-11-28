# PA-220 OEM Stage-3 and rebuilt U-Boot FDT comparison

Read-only comparison of the original SPI Stage-3 FDT and the FDT embedded in the rebuilt PA-220 U-Boot image.

| Field | OEM SPI Stage 3 | Rebuilt U-Boot |
|---|---:|---:|
| FDT offset | 0x1c9460 | 0x113700 |
| FDT size | 10850 | 10277 |
| Node count | 59 | 52 |
| Model | pan,pa-220 | pan,pa-220 |
| Compatible | cavium,tb7100 | pan,pa-220 | cavium,tb7100 |

## MMC controller

OEM: [{"path": "/soc@0/mmc@1180000002000", "compatible": "cavium,octeon-6130-mmc", "reg": "\u0001\u0018 |   | \u0001 | \u0001\u0018 | \u0001h |  ", "status": "ABSENT"}, {"path": "/soc@0/mmc@1180000002000/mmc-slot@0", "compatible": "cavium,octeon-6130-mmc-slot | mmc-spi-slot", "reg": "cells:00000000", "status": "ABSENT"}]
Rebuilt: [{"path": "/soc@0/mmc@1180000002000", "compatible": "cavium,octeon-6130-mmc", "reg": "\u0001\u0018 |   | \u0001 | \u0001\u0018 | \u0001h |  ", "status": "ABSENT"}, {"path": "/soc@0/mmc@1180000002000/mmc-slot@0", "compatible": "cavium,octeon-6130-mmc-slot | mmc-spi-slot", "reg": "cells:00000000", "status": "ABSENT"}]

## CN7130 USB controller

OEM: [{"path": "/soc@0/interrupt-controller@107000000e800", "compatible": "cavium,octeon-7130-cib", "reg": "\u0001\u0007 | \ufffd | \b | \u0001\u0007 | \ufffd | \b", "status": "ABSENT"}, {"path": "/soc@0/interrupt-controller@107000000e900", "compatible": "cavium,octeon-7130-cib", "reg": "\u0001\u0007 | \ufffd | \b | \u0001\u0007 | \ufffd | \b", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180068000000", "compatible": "cavium,octeon-7130-usb-uctl", "reg": "\u0001\u0018 | h | \u0001", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180068000000/xhci@1680000000000", "compatible": "cavium,octeon-7130-xhci | synopsys,dwc3", "reg": "\u0001h | \u0010", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180069000000", "compatible": "cavium,octeon-7130-usb-uctl", "reg": "\u0001\u0018 | i | \u0001", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180069000000/xhci@1690000000000", "compatible": "cavium,octeon-7130-xhci | synopsys,dwc3", "reg": "\u0001i | \u0010", "status": "ABSENT"}]
Rebuilt: [{"path": "/soc@0/interrupt-controller@107000000e800", "compatible": "cavium,octeon-7130-cib", "reg": "\u0001\u0007 | \ufffd | \b | \u0001\u0007 | \ufffd | \b", "status": "ABSENT"}, {"path": "/soc@0/interrupt-controller@107000000e900", "compatible": "cavium,octeon-7130-cib", "reg": "\u0001\u0007 | \ufffd | \b | \u0001\u0007 | \ufffd | \b", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180068000000", "compatible": "cavium,octeon-7130-usb-uctl", "reg": "\u0001\u0018 | h | \u0001", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180068000000/xhci@1680000000000", "compatible": "cavium,octeon-7130-xhci | synopsys,dwc3", "reg": "\u0001h | \u0010", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180069000000", "compatible": "cavium,octeon-7130-usb-uctl", "reg": "\u0001\u0018 | i | \u0001", "status": "ABSENT"}, {"path": "/soc@0/uctl@1180069000000/xhci@1690000000000", "compatible": "cavium,octeon-7130-xhci | synopsys,dwc3", "reg": "\u0001i | \u0010", "status": "ABSENT"}]

## Structural differences

Nodes only in rebuilt: /memory, /soc@0/i2c@1180000001000/temp-sensor@4c
Nodes only in OEM: /gpio-leds, /soc@0/i2c@1180000001000/fan-ctlr@2c, /soc@0/i2c@1180000001000/tlv-eeprom@56, /soc@0/i2c@1180000001000/tmp@48, /soc@0/i2c@1180000001000/tmp@49, /soc@0/i2c@1180000001000/tmp@4a, /soc@0/i2c@1180000001000/tmp@4b, /soc@0/i2c@1180000001000/tmp@4c, /soc@0/i2c@1180000001000/volt-monitor@35
Critical property differences: 4
- / compatible: OEM=cavium,tb7100; rebuilt=pan,pa-220 | cavium,tb7100
- /soc@0/mmc@1180000002000 power-gpios: OEM= | ; rebuilt= | 
- /soc@0/uctl@1180068000000 power: OEM= | ; rebuilt= | 
- /soc@0/uctl@1180069000000 power: OEM= | ; rebuilt= | 

## Toolchain bootinfo ABI

- cvmx-bootinfo.h not found in extracted toolchain

## Limits

- FDT similarity alone does not prove Linux accepted the passed FDT.
- The live kernel message 'Using internal Device Tree.' is separate evidence of fallback selection.
- No device access or SPI/eMMC writes are performed.

Full machine-readable inventory: boot-handoff-audit.json
