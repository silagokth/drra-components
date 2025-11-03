# Build and install SST CORE
wget https://github.com/sstsimulator/sst-core/releases/download/v14.1.0_Final/sstcore-14.1.0.tar.gz -O sstcore-14.1.0.tar.gz
tar -xzf sstcore-14.1.0.tar.gz
mv sst-core "$SST_CORE_ROOT"
cd "$SST_CORE_ROOT" || exit
./configure --prefix="$SST_CORE_HOME" --disable-mpi && make -j && make install

export PATH=$SST_CORE_HOME/bin:$PATH
export SST_DIR=$SST_CORE_HOME/lib/cmake/SST

# Build and install SST Elements (only the ones that are needed)
wget https://github.com/sstsimulator/sst-elements/releases/download/v14.1.0_Final/sstelements-14.1.0.tar.gz
tar -xzf sstelements-14.1.0.tar.gz
mv sst-elements "$SST_ELEMENTS_ROOT"
cd "$SST_ELEMENTS_ROOT" || exit
for folder in $(ls -d ./src/sst/elements/*/); do
  touch "$folder"/.ignore
done
rm -rf src/sst/elements/memHierarchy/.ignore
./autogen.sh
./configure --prefix="$SST_ELEMENTS_HOME" && make -j && make install

export PATH=$SST_ELEMENTS_HOME/bin:$PATH
