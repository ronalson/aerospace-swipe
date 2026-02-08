# Security-Hardened Local Fork Plan + First-Time C Build Runbook for `aerospace-swipe`

## Summary
This document defines a secure-by-default hardening and operations plan for a personal macOS fork of `aerospace-swipe`.

Defaults for this fork:
- Maximum local security over compatibility.
- Socket-only AeroSpace communication (no CLI shell fallback).
- Local source build/install workflow with verification steps (no `curl | bash`).

## 1) Scope and Threat Model

### In Scope
- Local use on a personal macOS machine.
- Reducing command-injection and memory-safety risk in core runtime paths.
- Making build/install/restart/uninstall reproducible for a first-time C user.

### Trust Boundary
- Accessibility permission is the highest-risk trust boundary for this app.
- If the process is compromised, Accessibility trust can be abused by an attacker.

### Non-Goals
- This is not a macOS App Sandbox hardening effort.
- This is not root-daemon or system-wide deployment hardening.
- This does not replace AeroSpace security posture; it hardens this client.

## 2) Current Risk Snapshot (Code-Grounded)

### A) Shell fallback risk (Critical)
- File: `src/aerospace.c`
- Prior behavior used shell command construction + `popen()` fallback when socket connect failed.
- Risk: command execution surface and shell interpretation risk.

### B) Touch-count bounds risk (High)
- Files: `src/main.m`, `src/event_tap.h`
- Gesture context arrays are fixed at `MAX_TOUCHES`, while runtime touch count can vary.
- Risk: out-of-bounds indexing if loops trust unchecked runtime count.

### C) Config validation risk (Medium)
- File: `src/config.h`
- Config values were accepted with limited range checking.
- Risk: malformed values causing unstable behavior or edge-case runtime bugs.

### D) Install guidance risk (Medium)
- File: `README.md`
- Existing script-based install guidance encourages remote execution patterns.
- Risk: supply-chain and operator safety issues.

## 3) Hardening Changes Required

### H1) Command Path Hardening (Critical)
- Enforce socket-only mode.
- Remove CLI shell fallback path entirely.
- On socket-connect failure, fail closed with a clear actionable error message.

### H2) Bounds Hardening (High)
- Clamp touch `count` to `MAX_TOUCHES` before any gesture array indexing.
- Ensure all loops touching `prev_x`/`base_x` use clamped count.

### H3) Config Validation + Safe Defaults (Medium)
- Validate/clamp `fingers`, `swipe_tolerance`, and numeric gesture thresholds.
- Accept only sane ranges; on invalid values, keep defaults and log warnings.
- Treat runtime config as validated effective config.

### H4) Build Hardening (Medium)
- Keep release target, add secure local debug profile:
  - `-O0 -g3 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fno-omit-frame-pointer`
- Add sanitizer build target for local validation:
  - `-fsanitize=address,undefined -fno-sanitize-recover=all`

### H5) Install Hardening (Medium)
- Use local source checkout and explicit build/install commands.
- Do not use `curl | bash` for hardened workflow.
- Keep LaunchAgent lifecycle explicit (`install`, `restart`, `uninstall`).

## 4) Beginner Build + Install Runbook (Copy/Paste)

### 4.1 Prerequisites
```bash
xcode-select -p
clang --version
make --version
```

If `xcode-select -p` fails:
```bash
xcode-select --install
```

### 4.2 Clone Your Fork and Build
```bash
git clone https://github.com/ronalson/aerospace-swipe.git
cd aerospace-swipe
make clean
make all
```

Expected result:
- `swipe` binary is built with no errors.

### 4.3 Build App Bundle + Sign
```bash
make bundle
```

Expected result:
- `AerospaceSwipe.app` exists in repo root.
- App is ad-hoc signed with `accessibility.entitlements`.

### 4.4 Install LaunchAgent
```bash
make install
```

What this does:
- Builds/updates app bundle.
- Writes `~/Library/LaunchAgents/com.acsandmann.swipe.plist`.
- Loads LaunchAgent into your user session.

### 4.5 Verify Runtime and Logs
```bash
launchctl list | grep com.acsandmann.swipe
ls -l /tmp/swipe.out /tmp/swipe.err
```

Then review logs:
```bash
tail -n 100 /tmp/swipe.out
tail -n 100 /tmp/swipe.err
```

### 4.6 Restart After Config Changes
```bash
make restart
```

### 4.7 Safe Rollback
```bash
make uninstall
```

## 5) Common Failures and Fixes

### A) Accessibility permission denied
Symptoms:
- Gestures do not trigger.

Fix:
1. Open System Settings -> Privacy & Security -> Accessibility.
2. Enable permission for `AerospaceSwipe.app`.
3. Run `make restart`.

### B) AeroSpace socket unavailable
Symptoms:
- Startup fails with explicit socket connection error.

Fix:
1. Start/repair AeroSpace.
2. Confirm expected socket exists for your user.
3. Restart with `make restart`.

### C) Toolchain missing
Symptoms:
- `clang`/`make` missing or Xcode path not configured.

Fix:
1. `xcode-select --install`
2. Re-run prerequisites checks.

## 6) Verification and Acceptance Criteria

### Security Acceptance
- No shell command fallback (`popen`) remains in command path.
- Socket failure behavior is explicit and fail-closed.

### Robustness Acceptance
- Touch count is bounded before indexing fixed arrays.
- No crashes when touch count spikes beyond supported max.

### Config Acceptance
- Out-of-range config values are clamped/rejected to defaults with warnings.
- Malformed config does not destabilize runtime.

### Operator Acceptance
- A first-time C user can build/install/verify/uninstall from this document alone.

## 7) Public Interface / Behavior Changes

### Aerospace Client Contract (`src/aerospace.c`)
- Initialization no longer implies CLI fallback availability.
- Socket-connect failure now ends initialization with an explicit error.

### Config Contract (`src/config.h`)
- Runtime config is validated effective config.
- Invalid inputs no longer flow unchecked into gesture logic.

### Operational Contract (Docs)
- Hardened local workflow uses source build + explicit verification steps.
- Remote script execution is not part of secure install guidance.

## 8) Test Cases and Scenarios

1. Build sanity:
```bash
make clean && make all
```

2. Bundle/sign sanity:
```bash
make bundle
```

3. Socket unavailable:
- Stop AeroSpace and start app; verify explicit socket error and no CLI fallback behavior.

4. Touch bounds:
- Exercise high-touch/rapid gesture input; verify stability and no crashes.

5. Invalid config values:
- Set extreme negative/large values in config; verify warnings and safe behavior.

6. Lifecycle:
```bash
make install
make restart
make uninstall
```

## 9) Execution Order and Commit Plan

1. Commit 1: socket-only command path hardening.
2. Commit 2: touch-count bounds guards.
3. Commit 3: config validation/clamping.
4. Commit 4: docs rewrite + beginner secure runbook.

For each commit, run and record:
```bash
make clean && make all
make bundle
```

## 10) Final Defaults for This Fork
- Target: local macOS machine, personal fork.
- Security posture: maximum local security.
- Fallback policy: socket-only.
- Documentation style: security-first brief plus practical runbook.
- Runtime model: user LaunchAgent (no root daemonization).
