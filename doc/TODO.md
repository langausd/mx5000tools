# MX5500 TODO

This document tracks the next steps for restoring MX5500-specific functionality that is not currently exposed by `mx5000tools`.

The primary target is the keyboard's built-in firmware menu, especially the Media and Inbox pages. The current library can replace the LCD contents with custom pages, but it does not know how to feed the original firmware pages with SetPoint-compatible data.

## Priority 1: Build a safe protocol test harness

- [ ] Add a small standalone test program for the historical/experimental menu-control code.
- [ ] Keep experimental commands out of the normal `mx5000-tool` CLI until their meaning is understood.
- [ ] Reuse `mx5000_open()` / `mx5000_send_report()` so tests work through the same hidraw transport as the existing library.
- [ ] Allow individual report sequences to be selected from the command line instead of sending all experiments at once.
- [ ] Print every outgoing report in hex before transmission.
- [ ] Add configurable delays between multi-report sequences; several existing keyboard settings already require ordered writes and delays.
- [ ] Always provide a clean `mx5000-tool --reset` / reconnect recovery path after an experiment.
- [ ] Document the observed result on an MX5500 for every tested command.

The original SetPoint installation is no longer available, so capturing a Windows/SetPoint session is not currently an option. Reverse engineering therefore has to start from already known Logitech report formats, historical code and controlled tests on the keyboard itself.

## Priority 2: Map the MX5500 firmware-menu protocol

- [ ] Inventory all known output reports and group them by command byte / report size (`0x10`, `0x11`, `0x12`).
- [ ] Separate commands that are confirmed on MX5500 from MX5000-only legacy commands.
- [ ] Determine which command(s) select or update individual built-in pages without switching to a custom LCD screen.
- [ ] Determine whether menu data is written as persistent configuration, transient state, or both.
- [ ] Check whether page changes generate useful input/HID reports that can be correlated with outgoing commands.
- [ ] Record minimum payloads and vary one byte/field at a time rather than brute-forcing whole reports.
- [ ] Add discovered fields and report layouts to a protocol-notes document before integrating them into `libmx5000`.

Known firmware pages to use as test targets:

1. Date / time
2. Media
3. Temperature
4. Favorites
5. Key counter
6. Inbox

The calculator is a separate keyboard-local mode and is not a seventh page.

## Priority 3: Restore the original Media page

- [ ] Identify the command that removes the "missing data" state from the built-in Media page.
- [ ] Determine the fields for title, artist and any other text shown by the firmware.
- [ ] Determine playback-state fields (playing/paused/stopped) if present.
- [ ] Determine whether progress/time information can be updated independently.
- [ ] Implement the protocol in `libmx5000` with a small, typed API instead of exposing raw reports to applications.
- [ ] Add corresponding diagnostic options to `mx5000-tool`.
- [ ] After the protocol is stable, add an optional Linux MPRIS bridge as a separate helper/daemon.

Fallback: custom rolling-text/progress-bar pages already work and can display MPRIS data, but they do not restore the native MX5500 Media page.

## Priority 4: Restore the original Inbox page

- [ ] Identify the command that removes the firmware's "Inbox missing" state.
- [ ] Determine whether the page accepts an unread count, sender/subject text, a status flag, or a combination of these.
- [ ] Correlate the page data with the already working E-mail icon command.
- [ ] Determine whether the icon and Inbox page are controlled by independent protocol fields.
- [ ] Implement the discovered protocol in `libmx5000` and expose a minimal CLI test interface.
- [ ] Keep mail-provider integration outside `libmx5000`; the library should only represent keyboard capabilities.

## Priority 5: Favorites and other firmware configuration

- [ ] Determine how the built-in Favorites page is populated/configured.
- [ ] Test whether the legacy `--name` command has a different MX5500 interpretation or is truly a no-op.
- [ ] Map all fixed LCD status icons on the MX5500 and compare them with the four legacy MX5000 icon fields.
- [ ] Check whether any MX5500-specific status icons can be controlled through currently unknown fields.
- [ ] Verify temperature-unit handling thoroughly on MX5500.

## Priority 6: Fix incomplete existing LCD functions

These do not block reverse engineering of the native menu, but should be made reliable while the code is being exercised heavily.

- [ ] Reproduce and fix unreliable `--update` handling for referenced text fields.
- [ ] Investigate why `huge` text does not render correctly on the MX5500 although the feature exists in the generic screen-content code.
- [ ] Test PBM image upload on the MX5500 and document the actual limits/placement behavior.
- [ ] Add regression tests for screen-content packet generation that do not require attached hardware.

## Priority 7: MX5500 capability handling

- [ ] Add an explicit device/capability distinction between MX5000 and MX5500 after opening the hidraw device.
- [ ] Avoid presenting known MX5000-only features as supported on MX5500.
- [ ] Keep legacy behavior available for MX5000 users.
- [ ] Document confirmed MX5500 differences in the public API and CLI help.

Currently observed MX5500 differences include:

- E-mail icon works.
- Mute icon works.
- Messenger and Phone/Walkie legacy icon fields appear to be ignored.
- `--name` appears to be ignored.
- normal-size custom text works; `huge` text currently does not.

## Priority 8: Reconnect and integration work

- [ ] Finalize and test automatic `--time` synchronization after Bluetooth reconnect.
- [ ] Verify behavior across repeated Bluetooth disconnect/reconnect cycles.
- [ ] Systematically capture all MX5500 special-key input events.
- [ ] Decide which keys should remain ordinary Linux input and which deserve optional helper integrations.

These tasks are useful for day-to-day Linux operation but should not be mixed with the low-level firmware-menu reverse engineering.

## Suggested implementation order

1. Protocol test harness for the historical menu code.
2. Controlled MX5500 test matrix and protocol notes.
3. Native Media page.
4. Native Inbox page.
5. Favorites and MX5500-specific icons/settings.
6. `--update`, `huge` text and PBM reliability fixes.
7. Capability-aware public API/CLI cleanup.
8. Optional MPRIS/mail/status daemons and reconnect automation.

## Definition of done for a reconstructed firmware feature

A feature should only be moved from experimental code into `libmx5000` when:

- the report sequence is reproducible after a fresh reconnect;
- each non-constant payload field has a documented purpose or is explicitly marked unknown;
- the feature has been tested repeatedly on an MX5500 without corrupting other keyboard settings/pages;
- a reset/recovery sequence is known;
- the library API describes the logical feature rather than exposing raw packet bytes;
- the behavior and remaining unknowns are documented in `doc/mx5500-status.md` or a dedicated protocol document.
