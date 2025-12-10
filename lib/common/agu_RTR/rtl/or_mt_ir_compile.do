set QUESTA_HOME /home/paul/local/siemens/2022-23/RHELx86/QUESTA-CORE-PRIME_2022.4/questasim
set QUESTA_GCC_BIN $QUESTA_HOME/gcc-7.4.0-linux_x86_64/bin
set QUESTA_GCC_LIB $QUESTA_HOME/gcc-7.4.0-linux_x86_64/lib64

set GCC "g++"
set CXX_FLAGS "-c -fPIC"
set INCLUDE_FLAGS "-I../../sst/include -I$QUESTA_HOME/include"
set LINKER_FLAGS "-shared"

exec $GCC -c -fPIC -I../../sst/include -I$QUESTA_HOME/include \
  ../../sst/src/timingModel.cpp -o timingModel.o
exec $GCC -c -fPIC -I../../sst/include -I$QUESTA_HOME/include \
  tm_wrapper.cpp -o tm_wrapper.o
exec $GCC $LINKER_FLAGS -o timingModel.so timingModel.o tm_wrapper.o \
  -static-libgcc -static-libstdc++

vlog -svinputport=var -sv ./agu_RTR_pkg.sv \
                          ./ir.sv \
                          ./mt_ir.sv \
                          ./or_mt_ir.sv \
                          ./or_mt_ir_tb.sv

vsim -sv_lib timingModel work.or_mt_ir_tb -voptargs="+acc" -debugDB
