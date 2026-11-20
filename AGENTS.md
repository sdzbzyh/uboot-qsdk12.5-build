# Repository Agent Instructions

These instructions apply to every assistant session working in this repository. Read this file before making changes.

## GitHub and local Debian workflow

The canonical shared state is the GitHub repository `sdzbzyh/uboot-qsdk12.5-build`, branch `feature/pa220-ubootkit-web`. Do not assume the local Debian checkout is synchronized with GitHub.

### Before every GitHub write

1. Fetch/read the current remote branch HEAD immediately before writing.
2. Preserve unrelated commits and working-tree changes. Never force-push.
3. After writing, read the changed file(s) back from GitHub and verify their contents and the resulting branch state.
4. If another session advances the branch during the operation, fetch again and reconcile; do not overwrite it.

### Preferred way to publish local Debian results

When a task requires execution in the Debian environment, make the command save its source, logs, and evidence directly under the designated repository path (normally `reports/pa220/<phase>/`). If the local checkout has a valid `origin` and the intended branch, the command should stage only the exact generated files, commit them, and attempt a normal push. Never stage the whole working tree indiscriminately.

If a normal push is rejected as non-fast-forward:

1. Do not force-push, reset, discard, or recreate the local commit. The local commit is preserved.
2. Fetch the intended branch:
   ```sh
   git fetch origin feature/pa220-ubootkit-web
   ```
3. Inspect the local commit and working tree. If rebase is blocked because untracked local files would be overwritten by files present in the remote commit, preserve only those colliding untracked files outside the checkout, retaining their relative paths. Use a unique backup directory under `/tmp`; for example:
   ```sh
   BACKUP="/tmp/pa220-p5-rebase-backup-<local-commit>"
   mkdir -p "$BACKUP"
   # For each exact conflicting untracked path:
   mkdir -p "$BACKUP/$(dirname "$path")"
   mv "$path" "$BACKUP/$path"
   ```
   Do not delete these files, do not move unrelated files, and do not assume that a whole directory is safe to move.
4. Rebase the preserved local commit onto the fetched remote branch:
   ```sh
   git rebase origin/feature/pa220-ubootkit-web
   ```
5. Only if rebase succeeds, push normally:
   ```sh
   git push origin HEAD:refs/heads/feature/pa220-ubootkit-web
   ```
6. Keep the backup until the resulting remote files have been checked. Do not copy old colliding files over newer remote-tracked versions. If rebase conflicts or fails for any reason, stop and inspect the state; do not run destructive cleanup or force-push.
7. Verify the push by reading the exact resulting files and current branch HEAD from GitHub. A local commit or successful local file creation is not proof of remote publication.

### Proven recovery sequence (2026-11-20)

The following sequence successfully published local commit `537087c2`:

- Normal push was rejected because GitHub had advanced to `a37608b6`.
- `git fetch origin feature/pa220-ubootkit-web` retrieved the newer remote commit.
- Rebase initially stopped because four untracked local files collided with paths now tracked remotely:
  - `patches/pa220/0002-pa220-board-clock-readonly-diagnostics.patch`
  - `reports/pa220/P5-1A.3/11-host-harness-inspection.txt`
  - `reports/pa220/P5-1A.3/12-host-stub-details.txt`
  - `reports/pa220/P5-1A.3/16-host-protocol-verification.txt`
- Those exact files were moved, not deleted, to `/tmp/pa220-p5-rebase-backup-537087c2/`, preserving relative paths.
- Rebase onto `origin/feature/pa220-ubootkit-web` then succeeded, followed by a normal non-forced push.
- GitHub branch subsequently advanced to `534b76d537d69a4bbc686ca1cd34338a14e5991e`.

This is a proven recovery procedure, not a command to move the same paths blindly in future sessions. First inspect the actual current branch, untracked paths, and exact rebase error; back up only files that truly collide.

## Safety rules for shell commands

- Avoid `exit`, `exec`-replacing-shell patterns, and `set -e` in interactive user terminal instructions; prior commands using them caused terminal sessions to close unexpectedly.
- Preserve diagnostic output and failure state. Use explicit conditional branches rather than terminating the interactive shell.
- Never use `git reset --hard`, `git clean -fd`, or force-push as a shortcut.
- Do not ask the user to manually inspect, copy, or upload repository files when GitHub integration can read or write them directly.
- Never claim that a local file has been published until GitHub confirms it.
