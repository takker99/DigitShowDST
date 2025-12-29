# Copilot Instructions for DigitShowBasic

## System Overview
- Legacy MFC single-doc/single-view app (`DigitShowBasicDoc`/`DigitShowBasicView`); the Doc keeps hardware state and globals, the View owns UI and timers.
- Timer 1 (50 ms) refreshes the UI: `AD_INPUT()` → `Cal_Physical()` → `Cal_Param()` updates `variables::physical::latest_physical_input` / `latest_physical_output`, and UI consumers read those snapshots directly.
- Timer 2 (500 ms) is the closed-loop controller; `Control_DA()` fans out to `MLoading_*`, `CLoading_*`, etc., and issues D/A writes.
- Timer 3 (default 1000 ms) streams logs through `SaveToFile()`; FIFO dumps use `SaveToFile2()` and reuse the same column order.

## Hardware & Calibration
- CONTEC access lives in `Caio.h`; expect device IDs `AIO000/1/2`, `AioSetAiRangeAll(..., 0)` for ±10 V, and `AioMultiAo` for batch analog output.
- Analog inputs are wired on every other channel—index using `AdChannels[i] / 2` and `2 * j` stride (see `AD_INPUT()` and `Cal_Physical()`).
- Calibration is quadratic (`Cal_a/b/c`) on A/D and linear (`DA_Cal_a/b`) on D/A; dialogs in `CalibrationFactor*.cpp` update persistence, Doc arrays hold runtime state.
- Offline runs must guard `Aio*` calls (`OpenBoard`, `AD_INPUT`, `DA_OUTPUT`); reuse existing `Flag_SetBoard` checks before touching hardware.

## Control Scripts & Logic
- **JSON bridge format** (active): Control dialog loads `.ctl.json` files validated against `schemas/control_script.schema.json`. Each step's `use` field maps to a `CFNUM` (see `knowledge/control_specifications.md`).
- `Control_File.cpp` converts JSON steps → `CFNUM`/`CFPARA[0..17]` arrays; `Control_DA()` dispatches to legacy helpers (`Control_MLoading.cpp`, `Control_CLoading.cpp`, etc.).
- **YAML end-state** (roadmap): Strategy extraction will enable YAML workflows with guards, metadata, and richer composition. JSON remains production until that work lands.
- When editing control logic, update `knowledge/control_specifications.md` first—it documents the `use` → CFNUM mapping, parameter units, and CF index assignments.

## Configuration & Schemas
- Calibration, specimen, and control data flow through JSON manifests in `schemas/` (validated via JSON Schema Draft 7).
- Runtime state split by concern: `Board.hpp`, `Control.hpp`, `Sensitivity.hpp`, `File.hpp`, `Variables.hpp` (PR #16).
- VS Code integrates schema validation—add `"$schema": "../schemas/control_script.schema.json"` to JSON files for autocomplete.

## Data Logging
- **TSV (stable)**: Since 2025-10-28 the app writes `{basename}.tsv`, `{basename}_vlt.tsv`, `{basename}_out.tsv` with `UnixTime(ms)` column; older `.dat/.vlt/.out` stay readable.
- **SQLite (prototype)**: PR #18 added dual-write to `{basename}.sqlite` (tables: `samples`, `voltages`). Schema is evolving; TSV remains the primary analysis source until Python tooling catches up.
- `SaveToFile()` captures `Phyout` + the parameter columns derived from `latest_physical_input` / `latest_physical_output` every Timer 3 tick; FIFO exports (`OnBUTTONWriteData` → `SaveToFile2()`) reconstruct timestamps from `FifoStartTimeMs`.
- `_out.tsv` headers are historical—consult `knowledge/data_file_formats.md` to map indices to actual quantities before renaming or reordering columns.

## Dependencies & Toolchain
- **vcpkg manifest mode**: `vcpkg.json` declares dependencies (nlohmann-json, doctest, ryml); MSBuild integration auto-installs to `vcpkg_installed/`.
- New dependencies: add to `vcpkg.json`, then rebuild. vcpkg handles headers/libs automatically.
- Submodule at `vcpkg/` pins the toolchain version; update via `git submodule update --remote vcpkg`.

## Build & Runtime
- Build through VS Code tasks (`MSBuild: Build Release`, `MSBuild: Build Debug`), which bootstrap `Launch-VsDevShell.ps1` automatically.
- Manual builds: `msbuild .\DigitShowBasic.sln /t:Build /p:Configuration=Release` (requires VS Dev Shell initialized via `Launch-VsDevShell.ps1`).
- No automated test harness yet; validate by exercising the GUI. Add hardware fallbacks (check `Flag_SetBoard`) whenever touching real I/O so agents can run without boards.

## Python Tooling
- Data analysis scripts live in `scripts/`; managed via `uv` and `pyproject.toml` (e.g., `uv run .\scripts\analyze_data.py <log_dir>`).
- `scripts/load_data.py` → `read_all_files()` auto-detects TSV vs legacy headers and returns pandas DataFrames (DatetimeIndex for new logs, `Time(s)` index for old).
- Log files are tab-delimited UTF-8 with BOM—match that encoding when exporting or diffing in Python utilities. SQLite loader support is roadmapped.

## Knowledge Base (`knowledge/`)
- **Start here** when touching control dispatch, configuration formats, logging, or refactor milestones. Each document begins with a status line ("Active", "Planned", etc.).
- Key guides: `control_specifications.md` (JSON → CFNUM mapping), `control_mechanisms_overview.md` (Timer 2 dispatch), `data_file_formats.md` (TSV/SQLite columns).
- Roadmap: `refactor_plan_2025-10-29.md` outlines staged modernization; `workspace_concept.md` tracks planned manifest system.
- Update the relevant doc when changing behavior—avoid creating new memos that scatter guidance.

## Key References
- Core flows: `DigitShowBasicDoc.cpp` (hardware, logging, control dispatch), `DigitShowBasicView.cpp` (timers/UI glue), and the `Control_*` modules (mode-specific logic).
- Dialog wiring sits in `MainFrm.cpp`/`DigitShowBasic.rc`; accelerators for menu shortcuts are maintained there as well.
- Additional context lives in `knowledge/data_file_formats.md`, `knowledge/tsv_unix_time_migration.md`, and `knowledge/code_analysis_2025-10-28.md`.
