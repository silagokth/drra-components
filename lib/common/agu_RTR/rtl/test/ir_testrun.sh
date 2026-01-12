#!/bin/bash

# Check if the -it argument is provided
interactive="-c"
arguments="$@"
if [[ " $arguments " == *" -it "* ]]; then
  interactive=""
fi

# Check if QUESTA_HOME is set
if [ -z "$QUESTA_HOME" ]; then
  echo "Error: QUESTA_HOME environment variable is not set."
  exit 1
fi

# Get current script directory
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

vsim $interactive -sv_lib \
  "${script_dir}/timingModel" \
  -work work \
  -do "set QUESTA_HOME ${QUESTA_HOME}; set ROOT ${script_dir}; do ${script_dir}/ir_compile.do; run -all; quit"
