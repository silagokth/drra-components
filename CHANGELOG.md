# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

### Added

- Added [sram:gf22.sv](lib/common/sram/rtl) & [sram_tb.sv](lib/common/sram/tb) ([5c1b9e0](https://github.com/silagokth/drra-components/pull/55/commits/5c1b9e041b5ec04037388d17495b7b0cf25679f1))

### Changed

- Updated [CODEOWNERS](./.github/CODEOWNERS) ([2d1d7c8](https://github.com/silagokth/drra-components/commit/2d1d7c84397d7efc6f28b3f6d66486bc3c031c6a))

## [2.2.0] - 2025-04-25

### Added

- Created [CHANGELOG.md](CHANGELOG.md)

### Changed

- **BREAKING CHANGE** Renamed `vesyla-suite` to `vesyla` based on [silagokth/vesyla@9a11d72](https://github.com/silagokth/vesyla/commit/9a11d72afe87d19b2a419d83e41330bed0403ed0) ([d03937f](https://github.com/silagokth/drra-components/commit/d03937f8b0a7369db92347b0d58cdc4cd5358dcc))
- Updated [README.md](README.md) ([ff0c314](https://github.com/silagokth/drra-components/commit/ff0c314ff7bbb64ceae9e404276fa07f7e911da5))
  - indicating GLIBC version
  - separating the prerequisites between "using pre-built release" or "building from source"
  - added more details on how to use the optional sccache package
- Updated release github workflow to include changelog in the release ([e5e0eee](https://github.com/silagokth/drra-components/commit/e5e0eeec1406410edcf9364e8b45422501795437))

### Removed

- Unnecessary comments in ci-build-base workflow ([836ff8b](https://github.com/silagokth/drra-components/commit/836ff8be44d7c68102e439a0608539c987649977))

## [2.1.4] - 2025-04-22
