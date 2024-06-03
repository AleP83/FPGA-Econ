#pragma once
#ifdef FPGA_MODE
#include "xcl2.hpp"
#endif

#include <array>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>

#include <time.h>
#include <math.h>
#include <cstdint>
#include <chrono>
#include <cassert>
#include <ctime>

#include "hw.h"
#ifdef OMPI_MODE
#include "mpi.h"
#endif 
#include "init.h"
#include "definitions.h"
#include "stopwatch.h"
#include "dev_options.h"
