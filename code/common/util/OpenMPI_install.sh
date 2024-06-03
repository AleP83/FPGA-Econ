#!/bin/bash

echo "Starting installation"

#Add development tools to instance 
sudo yum groupinstall "Development Tools" -y

#Download
wget https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.1.tar.bz2

#Extract, configure, compile, and install
tar -jxf openmpi-4.1.1.tar.bz2
cd openmpi-4.1.1
./configure --prefix=$HOME/openmpi
make all
make install
rm openmpi-4.1.1.tar.bz2

#Adapt PATH and LD_LIBRARY_PATH
export PATH=$PATH:$HOME/openmpi/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/openmpi/lib

echo "Done"
