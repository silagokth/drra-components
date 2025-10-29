## Purpose
This file gives concise, actionable guidance to an AI coding assistant working in the DRRA Component Library repository.

## Big-picture architecture
- Top-level library code is under `lib/` split into logical areas: `cells/`, `common/`, `controllers/`, `fabric/`, `resources/`, and `tb/`.
- The SST simulation tests and integration points live in `sst_tests/` and are built as separate executables (see `sst_tests/CMakeLists.txt`).
- The build produces a `build/library` directory containing the compiled DRRA SST shared library and a `library/VERSION` file (see top-level `CMakeLists.txt`).

Key files to read first: `CMakeLists.txt`, `sst_tests/CMakeLists.txt`, `README.md`, and `lib/*/rtl/*.sv.j2` (RTL templates).

## Build & test workflows (explicit)
- Clone with submodules: `git clone --recurse-submodules ...` or `git submodule update --init`.
- Standard build (with SST enabled):

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

- Build without SST (fast / local dev):

```bash
mkdir build && cd build
cmake .. -DUSE_SST=OFF
cmake --build .
```

- Register library for SST (CMake defines a `register_drra` target which calls `sst-register`):
  - When SST is enabled the top-level CMake will copy the built shared lib under `build/library` and run `sst-register`.
- Run SST element tests locally (after building):

```bash
sst-test-elements -w "drra*"
```

- CI runs (refer to `.github/workflows/ci-run-tests.yml`) use a drra-tests repo, an AppImage of `vesyla`, and a venv; the CI sets:
  - `VESYLA_SUITE_PATH_COMPONENTS` -> points to the extracted `library` folder
  - `PATH` to include the downloaded `vesyla` AppImage
  - modules such as Questasim, vesyla, bender, sst are loaded in CI

## Project-specific conventions & patterns
- Component descriptions and generator inputs live beside RTL templates:
  - `lib/<component>/arch.json`, `lib/<component>/isa.json`, `lib/<component>/tech.*.json` and a `timing_model` folder if present.
- RTL is generated via Jinja templates. See `lib/*/rtl/*.sv.j2` — do not change the Jinja call signatures; edit the module content only.
- Resources: resource components in `lib/resources/*` vary by number of slots; when adding/modifying resources, update input/output port definitions to match the intended slot count.
- Tests: every `*.cpp` in `sst_tests/` is added as an executable by `sst_tests/CMakeLists.txt`. Add new SST test programs by creating a `.cpp` there.

## Integration points / external deps to be aware of
- vesyla (AppImage in CI) — used to generate testcases and run flows. See `VESYLA_SUITE_PATH_COMPONENTS` usage in CI.
- SST framework — must set `SST_CORE_HOME` and `SST_ELEMENTS_HOME` in the environment for builds that use SST. `sst-config --includedir` is used by tests.
- gtest is required for building `sst_tests` (CI installs `requirements.txt` and the CMake finds GTest).
- `sccache` and Rust may be used; the README describes configuring `RUSTC_WRAPPER` for faster Rust builds.

## Common pitfalls & hints
- If you only need to iterate on RTL or non-SST code, use `-DUSE_SST=OFF` to avoid long SST link/build steps.
- The project sets static linking flags and an RPATH — linking failures often mean missing system libs or incompatible GLIBC (README notes GLIBC 2.35+ for releases).
- CI uses the `drra-tests` repository; locally reproducing CI may require installing Questasim/ModelSim and other modules or running inside a similar module environment.

## Examples (where to change things)
- Add a new cell component: create `lib/cells/my_cell/` with `arch.json`, `isa.json`, `rtl/` (Jinja), and `timing_model` as needed.
- Add a new SST test: drop `my_test.cpp` into `sst_tests/` — CMake will auto-add it as an executable.
- To inspect an existing FFT simulation test: see `drra-fft/sst_test/rf_fft_r4_sst/sst/rf_fft_r4.cpp` and corresponding files under `drra-fft/implementation/rtl/`.

## What this file expects from an AI assistant
- Prefer concrete, repo-aware edits (modify `rtl/*.sv.j2`, update `arch.json`, or add `sst_tests/*.cpp`) rather than broad refactors.
- When changing build logic, update `CMakeLists.txt` and ensure `build/library/VERSION` still writes as before.
- If adding or changing tests, validate by building locally (with SST when needed) and by matching the CI steps in `.github/workflows/ci-run-tests.yml`.

If anything here is unclear or you'd like more examples (e.g., a step-by-step for adding a new resource or running a single SST test locally), tell me which area to expand.
