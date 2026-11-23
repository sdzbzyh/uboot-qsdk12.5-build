# P5-2B API fields and data provenance
| URI | Fields | Source |
|---|---|---|
| /api/status | state, uptime_ms | get_timer(0), same millisecond timer used by the PA-220 TCP lifecycle |
| /api/board | board, board_type, cpu, cpu_clock_hz | PA-220 board file identifies board type 20021; CN7130 from the PA-220 board/config; gd->cpu_clk |
| /api/memory | dram_start_bytes, dram_total_bytes, dram_total_mib | gd->bd->bi_memstart and gd->bd->bi_memsize; total is DRAM capacity, not free memory |
| /api/network | interface, device_state, mac, ip | eth_get_dev()->name/state/enetaddr and NetOurIP; no environment variables are read |
- MAC is emitted only if nonzero, non-broadcast and unicast.
- Interface state is the U-Boot driver's INIT/PASSIVE/ACTIVE state, not a claim about physical carrier/link.
- Missing Ethernet device, invalid MAC or unconfigured IP is represented as JSON null/unavailable, never fabricated.
- Board and CPU names are fixed hardware identifiers from the pinned PA-220 board support. Serial number is intentionally omitted.
