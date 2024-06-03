#ifdef _OPENMPI_MODE
    #define OMPI_MODE			1       // 1 ON, 0 OFF
#elif _FPGA_MODE
    #define FPGA_MODE      		1       // 1 ON, 0 OFF
#elif _SERIAL_CPU_MODE
    #define SERIAL_CPU_MODE     1       // 1 ON, 0 OFF
#endif

#include "app.h"
using std::array;
using std::map;
using std::vector;

#if SERIAL_CPU_MODE

int main(){
    #if DETAILED_PERF_METRICS
    stopwatch total_time;
    stopwatch write_results_time;
    #endif

    // Start the stopwatch for measuring total time
    #if DETAILED_PERF_METRICS
    total_time.start();
    #endif

	preinit_t hw_preinit;
	env_t env;
	out_t out;
	
	init_all(&env);
	for(int i=0; i<J; i++){
		hw_preinit.k[i] = env.k[i];
	}
	runOnfpga(&hw_preinit, &out);

	#if DETAILED_PERF_METRICS
	write_results_time.start();
	#endif
	
	// Print 
	FILE *cfile;
	char FileName[512];
	printf("Migrating buffers from kernel\n");
	// Printing the Accumulation
	sprintf(FileName, "%sserial-cpu_nk%d.txt", CPU_REDUCE_OUT_FILE,J);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", out.reduced_sum);
	fclose(cfile);
	
	#if DETAILED_PERF_METRICS
	write_results_time.stop();
	#endif
	
	// Free Memory
    #if DETAILED_PERF_METRICS
    total_time.stop();
    #endif

    double tot_time = total_time.latency() * double(1e-9);
//-----------------------------------------------Save results-----------------------------------------------
    // Save total time in .txt file
    std::cout << "Total latency of the loop is: " << total_time.latency()* double(1e-9) << "s." << std::endl;
    std::cout << "Total time to write results is: " << write_results_time.latency()*double(1e-9) << "s." << std::endl;

    sprintf(FileName, "%snk%d-time-tot.txt", CPU_EXEC_OUT_FILE, J);
    cfile = fopen(FileName, "w");
    fprintf(cfile, "%.15lf \n", tot_time);
    fclose(cfile);

    return 0;
}

#elif FPGA_MODE

cl::Program load_cl2_binary(cl::Program::Binaries, cl::Device device, cl::Context context);

int main(int argc, char **argv)
{
	printf("App start\n");
	#if DETAILED_PERF_METRICS
	stopwatch total_time;
	stopwatch opencl_init_time;
	stopwatch kernel_total_time;
	stopwatch kernel_init_time;
	stopwatch kernel_h2d_time;
	stopwatch kernel_task_time;
	stopwatch kernel_d2h_time;
	stopwatch kernel_flush_time;
	stopwatch write_results_time;
	#endif

	#if DETAILED_PERF_METRICS
	total_time.start();
	#else
		clock_t tic = clock();
	#endif

	#if DETAILED_PERF_METRICS
	opencl_init_time.start();
	#endif

	cl_int err = CL_SUCCESS;
    std::string binaryFile = argv[1];
	// ------------------------------------------------------------------------------------
    // Step 1: get_xil_devices() is a utility API which will find the Xilinx
    // ------------------------------------------------------------------------------------    
    auto devices = xcl::get_xil_devices();
    auto device_count = devices.size();
    int NUM_DEVICES = (int) device_count;

	// ------------------------------------------------------------------------------------
	// Step 1: One element per device
	// ------------------------------------------------------------------------------------
    vector<cl::Context> contexts(device_count);
    vector<cl::Program> programs(device_count);
    vector<cl::CommandQueue> queues(device_count);
    vector< vector<cl::Kernel> > kernels(device_count, vector<cl::Kernel>(NUM_KERNELS));
    vector<std::string> device_name(device_count);
    vector<cl::Program::Binaries> bins(device_count);
	vector<cl::Platform> platform;
    vector<unsigned char> fileBuf[device_count];
	OCL_CHECK(err, err = cl::Platform::get(&platform));

	/* calculate the sizes of input and output buffers to be initiated*/
	//																												 ******** MODIFY FROM HERE
	const size_t hw_preinit_size_bytes = sizeof(preinit_t);																
	const size_t hw_out_size_bytes = sizeof(out_t);
	/*Initialize all buffers */
	vector<vector<preinit_t> > hw_preinit(NUM_DEVICES, vector<preinit_t> (NUM_KERNELS));
	vector<vector<out_t> >  hw_out(NUM_DEVICES, vector<out_t> (NUM_KERNELS));
	// Create opencl buffers
	vector< vector<cl::Buffer> > buffer_preinit(device_count, vector<cl::Buffer>(NUM_KERNELS));	
	vector< vector<cl::Buffer> > buffer_out(device_count, vector<cl::Buffer>(NUM_KERNELS));	
	//																												 ******** MODIFY TO HERE
	// Events for synchronization
	vector< vector< vector<cl::Event> >> memory_read_events(NUM_DEVICES, vector< vector<cl::Event> >(NUM_KERNELS, std::vector<cl::Event>(1)));
    vector< vector< vector<cl::Event> >> task_events(NUM_DEVICES, vector< vector<cl::Event> >(NUM_KERNELS, std::vector<cl::Event>(1)));
	vector< vector< vector<cl::Event> >> memory_write_events(NUM_DEVICES, vector< vector<cl::Event> >(NUM_KERNELS, std::vector<cl::Event>(1)));
    
	cl_context_properties props[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)(platform[0])(), 0};
    std::cout << "Initializing OpenCL objects" << std::endl;

	for (int d = 0; d < (int)device_count; d++) {
		// We create a context for each of the devices
        std::cout << "Creating Context[" << d << "]..." << std::endl;
        OCL_CHECK(err, contexts[d] = cl::Context(devices[d], props, nullptr, nullptr, &err));
        OCL_CHECK(err, queues[d] = cl::CommandQueue(contexts[d], devices[d], CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));
        OCL_CHECK(err, device_name[d] = devices[d].getInfo<CL_DEVICE_NAME>(&err));

        // read_binary_file() ia a utility API which will load the binaryFile and will return pointer to file buffer.
        fileBuf[d] = xcl::read_binary_file(binaryFile);
        bins[d].push_back({fileBuf[d].data(), fileBuf[d].size()});
        programs[d] = load_cl2_binary(bins[d], devices[d], contexts[d]);
		for (int k = 0; k < NUM_KERNELS; k++) {
			// 5 denotes up to 5 kernels per FPGA. Not used though
			if( k% 5 == 0 ){
				OCL_CHECK(err, kernels[d][k] = cl::Kernel(programs[d], "runOnfpga:{runOnfpga_1}", &err));	// ************ CHANGE NAMING
			}
			if( k% 5 == 1 ){
				OCL_CHECK(err, kernels[d][k] = cl::Kernel(programs[d], "runOnfpga:{runOnfpga_2}", &err));	
			}
			if( k% 5 == 2 ){
				OCL_CHECK(err, kernels[d][k] = cl::Kernel(programs[d], "runOnfpga:{runOnfpga_3}", &err));
			}
			// Allocate Buffers in Global Memory
			// Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and Device-to-host communication
			std::cout << "Creating Buffers[" << d << "] [" << k << "]..." << std::endl;
			//																												 ******** MODIFY FROM HERE
			OCL_CHECK(err, buffer_preinit[d][k] = cl::Buffer(contexts[d], CL_MEM_USE_HOST_PTR	| CL_MEM_READ_ONLY, hw_preinit_size_bytes, &hw_preinit[d][k], &err));	// ************ check read and write
			OCL_CHECK(err, buffer_out[d][k]		= cl::Buffer(contexts[d], CL_MEM_USE_HOST_PTR	| CL_MEM_WRITE_ONLY, hw_out_size_bytes, &hw_out[d][k], &err));			// ************ check read and write
			//																												 ******** MODIFY TO HERE
		}
    }
	
	// Check arguments in runOnfpga (hw.cpp,hw.h) follow the same order as in here 
    for (int d = 0; d < NUM_DEVICES; d++) {
		for (int k = 0; k < NUM_KERNELS; k++) {
			//																												 ******** MODIFY FROM HERE
			OCL_CHECK(err, err = kernels[d][k].setArg(0, buffer_preinit[d][k]));
			OCL_CHECK(err, err = kernels[d][k].setArg(1, buffer_out[d][k]));
			//																												 ******** MODIFY TO HERE
			std::cout << "Completed Setting Arguments"<< std::endl;
		}
	}

	#if DETAILED_PERF_METRICS
	opencl_init_time.stop();
	#endif

	#if DETAILED_PERF_METRICS
	kernel_total_time.start();
	#endif
	//do a for loop to initialize new data
	int COMP_PER_DEVICE = ceil(N_MODEL/(NUM_DEVICES*NUM_KERNELS));
	for(int i=0; i<COMP_PER_DEVICE; i++){	
		env_t env[NUM_DEVICES][NUM_KERNELS];
		
		/* copy the initialized data to new variables*/
		for(int d=0; d<NUM_DEVICES; d++){
			for (int k = 0; k < NUM_KERNELS; k++) {

				#if DETAILED_PERF_METRICS
				kernel_init_time.start();
				#endif

				init_all(&env[d][k]);
				//																												 ******** MODIFY FROM HERE
				for(int i=0; i<J; i++){
					hw_preinit[d][k].k[i] = env[d][k].k[i];
				}
				//																												 ******** MODIFY TO HERE
				#if DETAILED_PERF_METRICS
				kernel_init_time.stop();
				#endif

				// ------------------------------------------------------
				// Step 2: Copy Input data from Host to Global Memory on the device
				// ------------------------------------------------------
				#if DETAILED_PERF_METRICS
				kernel_h2d_time.start();
				#endif
				printf("Buffers to kernel\n");
				if(i == 0){

				// Memory Read Events in the first iteration i=0	
				OCL_CHECK(err,
					err = queues[d].enqueueMigrateMemObjects( {buffer_preinit[d][k]}, 
					 0 /* 0 means from host*/, nullptr, &memory_read_events[d][k][0])); 
				}
				else{
				// Memory Write Events (wait for events to end flag)
				OCL_CHECK(err,
					err = queues[d].enqueueMigrateMemObjects( {buffer_preinit[d][k]},  
					0 /* 0 means from host*/, &memory_write_events[d][k], &memory_read_events[d][k][0])); 	// wait for memory_write_events before launching new
				}
				#if DETAILED_PERF_METRICS
				kernel_h2d_time.stop();
				#endif
				// ----------------------------------------
				// Step 2: Submit Kernels for Execution
				// ----------------------------------------
				#if DETAILED_PERF_METRICS
				kernel_task_time.start();
				#endif
				printf("Enqueing Task\n");
				OCL_CHECK(err, 
					err = queues[d].enqueueTask(kernels[d][k], &memory_read_events[d][k], 
					&task_events[d][k][0]));
				#if DETAILED_PERF_METRICS
				kernel_task_time.stop();
				#endif
				// --------------------------------------------------
				// Step 2: Copy Results from Device Global Memory to Host
				// --------------------------------------------------
				#if DETAILED_PERF_METRICS
				kernel_d2h_time.start();
				#endif
				printf("Migrating buffers from kernel\n");
				OCL_CHECK(err,
					err = queues[d].enqueueMigrateMemObjects({buffer_out[d][k]}, 
					CL_MIGRATE_MEM_OBJECT_HOST, &task_events[d][k], &memory_write_events[d][k][0]));

				#if DETAILED_PERF_METRICS
				kernel_d2h_time.stop();
				#endif
			}
		}
		
		// Stope the Queues 
		for (int d = 0; d < NUM_DEVICES; d++) {
			stopwatch new_krnl_flush;
			#if DETAILED_PERF_METRICS
				kernel_flush_time.start();
				new_krnl_flush.start();
			#endif
			queues[d].finish();    
			#if DETAILED_PERF_METRICS
				kernel_flush_time.stop();
				new_krnl_flush.stop();
			#endif
			std::cout << "New kernel flush time is: " << new_krnl_flush.latency() * double(1e-9) << "s." << std::endl;
		}

		#if DETAILED_PERF_METRICS
		write_results_time.start();
		#endif
		
		for (int d = 0; d < NUM_DEVICES; d++) {
			for(int k=0; k < NUM_KERNELS; k++){

				FILE *cfile;
				char FileName[512];
				printf("Migrating buffers from kernel\n"); //add kgrid, km grid to file names
				// Printing the Value Function
				sprintf(FileName, "%sfpga_nk%d_d%d_k%d.txt", FPGA_REDUCE_OUT_FILE,J, d, k);
				cfile = fopen(FileName, "w");
				fprintf(cfile, "%.15lf \n", hw_out[d][k].reduced_sum);
				fclose(cfile);
			}
		}

		#if DETAILED_PERF_METRICS
		write_results_time.stop();
		#endif
		
	}

	#if DETAILED_PERF_METRICS
	kernel_total_time.stop();
	#endif
	
	// Performance metrics (main)
	#if DETAILED_PERF_METRICS
		total_time.stop();
	#else
		clock_t toc = clock();
		real totTime = (real)(toc - tic) / CLOCKS_PER_SEC;
	#endif

	real totTime = total_time.latency() * double(1e-9);
	#if DETAILED_PERF_METRICS
		std::cout << "Total latency of the loop is: " << totTime << "s." << std::endl;
		std::cout << "Total latency of opnecl initialization is: " << opencl_init_time.latency() * double(1e-9) << "s." << std::endl;
		std::cout << "Total latency of kernel total time is: " << kernel_total_time.latency() * double(1e-9) << "s." << std::endl;
		std::cout << "Total latency of kernel init time is: " << kernel_init_time.latency() * double(1e-9) << "s." << std::endl;
		std::cout << "Total latency of kernel h2d is: " << kernel_h2d_time.latency() * double(1e-9) << "s." << std::endl;
		std::cout << "Total latency of kernel task is: " << kernel_task_time.latency() * double(1e-9) << "s." << std::endl;
		std::cout << "Total latency of kernel d2h is: " << kernel_d2h_time.latency() * double(1e-9) << "s." << std::endl;
		std::cout << "Total latency of kernel flush time is: " << kernel_flush_time.latency() * double(1e-9) << "s." << std::endl;
		std::cout << "Total latency of writing results is: " << write_results_time.latency() * double(1e-9) << "s." << std::endl;
		std::cout << "---------------------------------------------------------------" << std::endl;
		std::cout << "Average latency of kernel execution per loop iteration is: " << kernel_task_time.avg_latency() * double(1e-9) << "s." << std::endl;
		std::cout << "Average latency of kernel flush per loop iteration is: " << kernel_flush_time.avg_latency() * double(1e-9) << "s." << std::endl;

		char* num_of_devices;
		if(NUM_DEVICES==1)
			num_of_devices = "I";
		else if (NUM_DEVICES==2)
			num_of_devices = "II";
		else if (NUM_DEVICES==8)
			num_of_devices = "III";

		FILE *cfile;
		char FileName[512];
		printf("\nWriting total kernel execution time to file\n");
		sprintf(FileName, "%sfpga%s-nk%d-time-tot.txt",FPGA_EXEC_OUT_FILE,num_of_devices,J);
		cfile = fopen(FileName, "w");
		fprintf(cfile, "%.15lf \n", totTime);
		fclose(cfile);
	#endif

	return 0;
}

cl::Program load_cl2_binary(cl::Program::Binaries bins, cl::Device device, cl::Context context) {
    cl_int err;
    std::vector<cl::Device> devices(1, device);
    OCL_CHECK(err, cl::Program program(context, devices, bins, nullptr, &err));
    return program;
}

#endif
