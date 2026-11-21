# P5-1B patch correction — new-file additions

The first dedicated workflow run `44860655890` failed before compilation because the generated 0003 diff incorrectly described `u-boot/net/tcp.c` and `u-boot/include/net/tcp.h` as modifications to existing files rather than additions. The pinned U-Boot tree has no such files yet.

The patch has now been regenerated from the exact pinned baseline files plus the existing integration TCP core. Both new files are emitted as proper Git additions (`new file mode 100644`, `--- /dev/null`). Existing 0001/0002 path layout is retained.

The corrected patch also contains the `tcp_core_link_anchor()` retention table required because the PA-220 final link uses `--gc-sections`. The next dedicated workflow run must confirm patch apply-check, target compilation, aggregate object symbols and final ELF/map/System.map symbols.

No local build or P5-1A Host regression was performed.
