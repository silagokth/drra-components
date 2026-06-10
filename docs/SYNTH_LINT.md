# Synthesizability lint — Tier A (open tools, no PDK)

The cheap, broad half of the synthesizability strategy. Assembles a stdcell-only
fabric and lints + elaborates it with **open tools only** (Verible + slang) on a
stock GitHub-hosted runner. No PDK, no liberty, no self-hosted server, no
confidential data — so it runs on **every PR**.

It is the complement to the gated Genus gate (`docs/SYNTH_CI.md`): synthesizability
breakage is PDK-independent, so most of it is caught here for free; the Genus gate
is reserved for authoritative area/Fmax sign-off on the PDK server.

## What it does

```
ci-synth-lint.yml (ubuntu-latest, every PR)
  ├─ install Verible (required) + slang (best-effort prebuilt)
  ├─ download the in-run `library` artifact + vesyla AppImage (public release)
  ├─ vesyla component assemble -i scripts/synth/arch_smoke_no_sram.json -o build
  │     (pure RTL composition — no solver, no simulation, top module `fabric`)
  └─ scripts/synth/lint_fabric.sh build/rtl
```

`vesyla component assemble` is the bare composition path (no minizinc / no
QuestaSim), which is why this works on a vanilla runner. The input
`scripts/synth/arch_smoke_no_sram.json` is a minimal 1×1 **no-SRAM** fabric
(swb + io + 3×rf + dpu on `cell_single_row`), so it is stdcell-only and needs no
SRAM macro.

## What it catches

| Tool | Check | Severity |
|------|-------|----------|
| `verible-verilog-syntax` | parse breakage from templating / bad SV | hard fail |
| `verible-verilog-lint` | style + a few structural rules (`scripts/synth/verible.rules`) | soft (hard if `STRICT=1`) |
| `slang --top fabric` | full elaboration: unresolved modules, port/width mismatches, and — because the RTL uses `always_comb` — incomplete-assignment **latches** | hard fail (when slang present) |

slang natively elaborates the `agu_cfg_if` SystemVerilog interfaces that plain
Yosys cannot, which is why it is the elaboration engine here.

slang is **best-effort**: if no prebuilt binary is published for the runner, the
step skips with a notice and Verible remains the guaranteed baseline. Verible has
reliable static Linux releases.

## Relationship to Tier B (Genus)

| | Tier A (this) | Tier B (`ci-synth.yml`) |
|--|--------------|--------------------------|
| Runner | GitHub-hosted | self-hosted w/ PDK server |
| PDK | none | GF22 (server-side only) |
| Catches | synth-construct breakage, latches | + real area / Fmax |
| Runs | every PR | gated (label + environment) |
| Cost | seconds–minutes | ~30 min + human gate |

## Local run

```sh
export VESYLA_SUITE_PATH_COMPONENTS=/path/to/built/library
vesyla component assemble -i scripts/synth/arch_smoke_no_sram.json -o build
bash scripts/synth/lint_fabric.sh build/rtl       # STRICT=1 to enforce lint too
```

## Notes / first-run

- Needs the built `library` artifact (produced in-run by `ci-build-library`) and
  the public vesyla AppImage release. The AppImage runs via
  `APPIMAGE_EXTRACT_AND_RUN=1` (no libfuse2 on the runner).
- If slang's prebuilt asset naming changes, adjust the grep in the install step.
- To extend coverage, add more checked-in arch variants (e.g. a 3×1 no-SRAM
  fabric) and lint each — still no PDK required.
