# PA-220 Command Test Logs

This directory stores raw command output collected during PA-220 U-Boot development and runtime validation.

## Rules

- Save command output here as plain text without editing or summarizing it.
- Keep each capture as a separate file.
- Use filenames in the form `YYYY-MM-DD_<phase-or-topic>_<sequence>.txt`.
- Include the exact commands executed and the complete returned output.
- Do not include passwords, private keys, access tokens, or other secrets.
- Put analysis, conclusions, and milestone summaries in `../reports/`, not in this directory.
- When a command produces no output or fails, preserve that fact and include the error output.

## Suggested topics

- `P3_tlv_reader`: source inspection and TLV/EEPROM command output.
- `P3_bootmem`: `bdinfo`, `namedprint`, `freeprint`, and `meminfo`.
- `P3_runtime`: U-Boot version, help, and `pa220` command output.

