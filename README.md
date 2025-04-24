# DRRA Component Library

This repository contains the DRRA component library. The library is a collection of components that can be used to build a DRRA application.

## Prerequisites

### For using [pre-built releases](https://github.com/silagokth/drra-components/releases)

- GLIBC 2.35 or newer
- [vesyla](https://github.com/silagokth/vesyla)
  - **Option 1**: From the [pre-built releases](https://github.com/silagokth/vesyla/releases)
  - **Option 2**: Built from [source](https://github.com/silagokth/vesyla)
- [SST framework](https://sst-simulator.org)
  - Install dependencies:
    - gtest (`sudo apt-get install libgtest-dev`)
  - [Detailed Build and Installation Instructions](https://sst-simulator.org/SSTPages/SSTBuildAndInstall_14dot1dot0_SeriesDetailedBuildInstructions/)
  - **Make sure that `SST_CORE_HOME` and `SST_ELEMENTS_HOME` are correctly set in your ENV**

### For building from source

- cmake
- make
- [rust](https://www.rust-lang.org/learn/get-started)
- (Optional, to run ISA-level simulations) [SST framework](https://sst-simulator.org)
- (Optional, to build faster) [sccache](https://lib.rs/crates/sccache). After installation, you'll need to configure it using one of these methods:
  - **Option 1**: Configure via environment variables:

    ```bash
    export RUSTC_WRAPPER=/path/to/sccache
    ```

  - **Option 2**: Configure globally by setting it in `$HOME/.cargo/config.toml`:

    ```toml
    [build]
    rustc-wrapper = "/path/to/sccache"
    ```

## Compilation and Installation

To compile the library, run the following commands:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

To compile the library _without SST_, run the following commands:

```bash
mkdir build
cd build
cmake .. -DUSE_SST=OFF
cmake --build .
```

The complete library will be compiled in the `build/library` directory. You can copy the folder to wherever you want to use the library.

## Running the SST Tests

To make sure that the simulation components are working correctly, you can run the tests by running the following command:

```bash
sst-test-elements -w "drra*"
```

## Usage

This library should be used by [`vesyla`](https://github.com/silagokth/vesyla). To let the program know where the library is located, you need to set the `VESYLA_SUITE_PATH_COMPONENTS` environment variable to the path of the library folder. For example, if you have copied the library to `/usr/local/lib/drra-component-library`, you can set the environment variable by running the following command

```bash
export VESYLA_SUITE_PATH_COMPONENTS=/usr/local/lib/drra-component-library
```

## Customization

You can add more custom components to the library by adding them to the `lib` directory. The easiest way to do this is to copy an existing component and modify it to suit your needs. You need to modify the following files:

- `arch.json`: The architecture description file.
- `isa.json`: The instruction set description file.
- `tech:X.json`: The technology description file (for example `tech:tsmc28.json`).
- `rtl/*.sv.j2`: The RTL Jinja2 templates description file. Note that, you should only modify the contents of the module description, not the Jinja calls. If the component is a resource ([./lib/resources](./lib/resources)), you should adjust the input and output ports definition based on the size (how many slots) of the resource.
- `timing_model`: The timing behavior of the component. Used by the instruction scheduler.
