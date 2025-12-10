set QUESTA_GCC_BIN $QUESTA_HOME/gcc-7.4.0-linux_x86_64/bin
set QUESTA_GCC_LIB $QUESTA_HOME/gcc-7.4.0-linux_x86_64/lib64

puts "QUESTA_HOME = $QUESTA_HOME"
puts "ROOT = $ROOT"

set GCC "g++"
set CXX_FLAGS "-c -fPIC"
set INCLUDE_FLAGS "-I$ROOT/../../../sst/include -I$ROOT/include"
set LINKER_FLAGS "-shared"

exec $GCC -c -fPIC -I$ROOT/../../../sst/include -I$QUESTA_HOME/include \
  $ROOT/../../../sst/src/timingModel.cpp -o $ROOT/timingModel.o
exec $GCC -c -fPIC -I$ROOT/../../../sst/include -I$QUESTA_HOME/include \
  $ROOT/tm_wrapper.cpp -o $ROOT/tm_wrapper.o
exec $GCC $LINKER_FLAGS -o $ROOT/timingModel.so $ROOT/timingModel.o $ROOT/tm_wrapper.o \
  -static-libgcc -static-libstdc++


vlog -svinputport=var -sv "$ROOT/../src/agu_RTR_pkg.sv" \
                          "$ROOT/../src/ir.sv" \
                          "$ROOT/ir_tb.sv"

vsim -sv_lib $ROOT/timingModel work.ir_tb -voptargs="+acc" -debugDB
