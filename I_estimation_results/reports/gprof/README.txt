- Log into m5n.large instance.
- Set the N_MODEL=10 for profiling
- Compile the c++ code.
g++ -D_SERIAL_CPU_MODE  -I./common -I./fpga -Wall -O3 -g -std=c++1y -fmessage-length=0 -pthread -lrt -lstdc++   common/init.cpp common/app.cpp fpga/hw.cpp -pg -o app
- Run the executable. This generates an additional gmon.out
./app
- Run the following command for getting the profiled result
gprof app gmon.out > cpu_analysis.txt