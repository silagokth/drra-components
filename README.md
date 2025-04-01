# DRRA Component Library

This repository contains the DRRA component library. The library is a collection of components that can be used to build a DRRA application.

## Prerequisites

This library has the following dependencies:

- cmake
- make
- [rust](https://www.rust-lang.org/learn/get-started)
- (Optional) [sccache](https://lib.rs/crates/sccache) (to build faster)

If you want to be able to run SST simulations:
- [SST framework](https://sst-simulator.org)
  - [Detailed Build and Installation Instructions](https://sst-simulator.org/SSTPages/SSTBuildAndInstall_14dot1dot0_SeriesDetailedBuildInstructions/)
  - **Make sure that `SST_CORE_HOME` and `SST_ELEMENTS_HOME` are correctly set in your ENV**
- gtest (`sudo apt-get install libgtest-dev`)


## Compilation and Installation

To compile the library, run the following commands:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

To compile the library without SST, run the following commands:

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

This library should be used by the [Vesyla-Suite](https://github.com/silagokth/vesyla-suite-4) program. To let the program know where the library is located, you need to set the `VESYLA_SUITE_PATH_COMPONENTS` environment variable to the path of the library. For example, if you have copied the library to `/usr/local/lib/drra-component-library`, you can set the environment variable by running the following command:

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
