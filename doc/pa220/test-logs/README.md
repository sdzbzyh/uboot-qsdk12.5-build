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



## Session-reconstructed evidence

Some historical command output can be recovered from the user's messages but is no longer available as a complete exported terminal capture. Such records use the suffix `.session-transcribed.txt` and must state their provenance and limitations at the top.

- These files are not raw device log captures.
- Preserve only text actually recoverable from the historical user message.
- Do not fill missing output with guesses or assistant-generated summaries.
- If only a summary survives, label it as a session-transcribed summary and keep the missing-original-evidence limitation explicit.
