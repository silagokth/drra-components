# DRRA FFT Components Library

This directory contains the FFT-specific components of the DRRA architecture.

## FFT-Specific Components

### Address Generation Units
- [`agu_fft`](./common/agu_fft)
  - Used in: `fft16_1cell` testcase
  - Module of [`rf_io`](./resources/rf_io) component
  - Handles address generation for radix-2 FFT
- [`agu_fft_r4`](./common/agu_fft_r4)
  - Used in: `fft_r4` and `fft_2r2` testcases
  - Module of [`rf_fft_r4`](./resources/rf_fft_r4) component
  - Handles address generation for radix-2 and radix-4 FFT

### Butterfly Units and Processing
- [`bu`](./resources/bu)
  - Used in: `fft_r4` and `fft_2r2` testcases
  - Implements radix-2 and radix-4 butterfly operations and integrates complex multiplier
- [`radix2_bu`](./resources/radix2_bu)
  - Used in: `fft16_1cell` testcase
  - Implements radix-2 butterfly operations
- [`complex_dpu`](./resources/complex_dpu)
  - Used in: `fft16_1cell` testcase
  - Handles complex number arithmetic operations

### Register Files
- [`rf_fft_r4`](./resources/rf_fft_r4)
  - Used in: `fft_r4` and `fft_2r2` testcases
  - Stores FFT data and manage FFT addressing for both radix-2 and radix-4 FFT, up to 256 points
- [`rf_io`](./resources/rf_io)
  - Used in: `fft16_1cell` single-cell testcase
  - Stores FFT data and manage FFT addressing for radix-2.
  - Has IO connections for single-cell implementation (no SRAM)

### Twiddle factor Generator
- [`tfg`](./resources/tfg)
  - Used in: `fft_r4` and `fft_2r2` testcases
  - Generates twiddle factors on-the-fly from base values saved in ROM