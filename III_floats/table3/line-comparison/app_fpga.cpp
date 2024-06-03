cl::Program load_cl2_binary(cl::Program::Binaries, cl::Device device, cl::Context context);

int main(int argc, char **argv)
{
	printf("K-S app start\n");
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
	#endif

	#if DETAILED_PERF_METRICS
	opencl_init_time.start();
	#endif

	cl_int err = CL_SUCCESS;
    std::string binaryFile = argv[1];
	
	auto devices = xcl::get_xil_devices();
    auto device_count = devices.size();
    int NUM_DEVICES = (int) device_count;

	vector<cl::Context> contexts(device_count);
    vector<cl::Program> programs(device_count);
    vector< vector<cl::Kernel> > kernels(device_count, vector<cl::Kernel>(NUM_KERNELS));
    vector<cl::CommandQueue> queues(device_count);
    vector<std::string> device_name(device_count);

    vector<cl::Program::Binaries> bins(device_count);
	vector<cl::Platform> platform;
    std::vector<unsigned char> fileBuf[device_count];
	OCL_CHECK(err, err = cl::Platform::get(&platform));

	int COMP_PER_DEVICE = ceil(N_MODEL/(NUM_DEVICES*NUM_KERNELS));

	/* calculate the sizes of all buffers to be initiated*/
	const size_t hw_iter_size = 300; // arbitrary number chosen to represent max iterations

	const size_t hw_preinit_size_bytes = sizeof(preinit_t);
	const size_t hw_out_size_bytes = sizeof(out_t);
	const size_t hw_iter_size_bytes = sizeof(int) * (hw_iter_size);

	/*Initialize all buffers */
	vector<vector<preinit_t> > hw_preinit(NUM_DEVICES, vector<preinit_t> (NUM_KERNELS));
	vector<vector<out_t> >  hw_out(NUM_DEVICES, vector<out_t> (NUM_KERNELS));
	vector<vector<vector<int, aligned_allocator<int>>>> hw_iter(NUM_DEVICES, vector< vector<int, aligned_allocator<int>> >(NUM_KERNELS, vector<int, aligned_allocator<int>>(hw_iter_size)));

	// Create opencl buffers
	vector< vector<cl::Buffer> > buffer_agshock(device_count, vector<cl::Buffer>(NUM_KERNELS));	
	vector< vector<cl::Buffer> > buffer_idshock(device_count, vector<cl::Buffer>(NUM_KERNELS));	
	vector< vector<cl::Buffer> > buffer_preinit(device_count, vector<cl::Buffer>(NUM_KERNELS));	
	vector< vector<cl::Buffer> > buffer_out(device_count, vector<cl::Buffer>(NUM_KERNELS));	
	vector< vector<cl::Buffer> > buffer_hw_iter(device_count, vector<cl::Buffer>(NUM_KERNELS));
	int total_egm_iter[NUM_DEVICES][NUM_KERNELS];


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
			if( k% 5 == 0 ){
				OCL_CHECK(err, kernels[d][k] = cl::Kernel(programs[d], "runOnfpga:{runOnfpga_1}", &err));
			}
			if( k% 5 == 1 ){
				OCL_CHECK(err, kernels[d][k] = cl::Kernel(programs[d], "runOnfpga:{runOnfpga_2}", &err));
			}
			if( k% 5 == 2 ){
				OCL_CHECK(err, kernels[d][k] = cl::Kernel(programs[d], "runOnfpga:{runOnfpga_3}", &err));
			}
			// Allocate Buffers in Global Memory
			std::cout << "Creating Buffers[" << d << "] [" << k << "]..." << std::endl;
			OCL_CHECK(err, buffer_agshock[d][k] = cl::Buffer(contexts[d], CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, (cl::size_type) AGSHOCK_ARR_SIZE, NULL, &err));
			OCL_CHECK(err, buffer_idshock[d][k] = cl::Buffer(contexts[d], CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, (cl::size_type) IDSHOCK_ARR_SIZE, NULL, &err));
			OCL_CHECK(err, buffer_preinit[d][k] = cl::Buffer(contexts[d], CL_MEM_USE_HOST_PTR	| CL_MEM_READ_ONLY, hw_preinit_size_bytes, &hw_preinit[d][k], &err));
			OCL_CHECK(err, buffer_out[d][k]		= cl::Buffer(contexts[d], CL_MEM_USE_HOST_PTR	| CL_MEM_WRITE_ONLY, hw_out_size_bytes, &hw_out[d][k], &err));
			OCL_CHECK(err, buffer_hw_iter[d][k] = cl::Buffer(contexts[d], CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, hw_iter_size_bytes, hw_iter[d][k].data(), &err));
		}
    }
	unsigned char* agshock_ptr[NUM_DEVICES][NUM_KERNELS];
	unsigned char* idshock_ptr[NUM_DEVICES][NUM_KERNELS];

    for (int d = 0; d < NUM_DEVICES; d++) {
		for (int k = 0; k < NUM_KERNELS; k++) {
			OCL_CHECK(err, err = kernels[d][k].setArg(0, buffer_agshock[d][k]));
			OCL_CHECK(err, err = kernels[d][k].setArg(1, buffer_idshock[d][k]));
			OCL_CHECK(err, err = kernels[d][k].setArg(2, buffer_preinit[d][k]));
			OCL_CHECK(err, err = kernels[d][k].setArg(3, buffer_out[d][k]));
			OCL_CHECK(err, err = kernels[d][k].setArg(4, buffer_hw_iter[d][k]));
			std::cout << "Comleted Setting Arguments"<< std::endl;
			agshock_ptr[d][k] = (unsigned char *) queues[d].enqueueMapBuffer(buffer_agshock[d][k], CL_TRUE, CL_MAP_WRITE, 0, AGSHOCK_ARR_SIZE);
			idshock_ptr[d][k] = (unsigned char *) queues[d].enqueueMapBuffer(buffer_idshock[d][k], CL_TRUE, CL_MAP_WRITE, 0, IDSHOCK_ARR_SIZE);
		}
	}

	#if DETAILED_PERF_METRICS
	opencl_init_time.stop();
	#endif

	#if DETAILED_PERF_METRICS
	kernel_total_time.start();
	#endif
	//do a for loop to initialize new data
	for(int i=0; i<COMP_PER_DEVICE; i++){	
		env_t env[NUM_DEVICES][NUM_KERNELS];
		input_t in[NUM_DEVICES][NUM_KERNELS];
		vars_t vars[NUM_DEVICES][NUM_KERNELS];

		/* copy the initialized data to new variables*/
		for(int d=0; d<NUM_DEVICES; d++){
			for (int k = 0; k < NUM_KERNELS; k++) {

				#if DETAILED_PERF_METRICS
				kernel_init_time.start();
				#endif

				init_all(&env[d][k], &in[d][k], &vars[d][k]);

				for(int i=0; i<NSTATES; i++){
					hw_preinit[d][k].kprime[i] = vars[d][k].kprime_a[i];
				}

				for(int i=0; i<NSTATES; i++){
					hw_preinit[d][k].wealth[i] = env[d][k].wealth[i];
				}

				memcpy(agshock_ptr[d][k], in[d][k].agshock, AGSHOCK_ARR_SIZE);
				memcpy(idshock_ptr[d][k], in[d][k].idshock, IDSHOCK_ARR_SIZE);

				#if DETAILED_PERF_METRICS
				kernel_init_time.stop();
				#endif

				#if DETAILED_PERF_METRICS
				kernel_h2d_time.start();
				#endif
				printf("Migrating buffers to kernel\n");
				if(i == 0){
				OCL_CHECK(err,
					err = queues[d].enqueueMigrateMemObjects( {
					 buffer_agshock[d][k], buffer_idshock[d][k], buffer_preinit[d][k] }, 
					 0 /* 0 means from host*/, nullptr, &memory_read_events[d][k][0])); 
				}
				else{
				OCL_CHECK(err,
					err = queues[d].enqueueMigrateMemObjects( {
					 buffer_agshock[d][k], buffer_idshock[d][k],buffer_preinit[d][k]  },  
					0 /* 0 means from host*/, &memory_write_events[d][k], &memory_read_events[d][k][0])); 
				}
				#if DETAILED_PERF_METRICS
				kernel_h2d_time.stop();
				#endif
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
				#if DETAILED_PERF_METRICS
				kernel_d2h_time.start();
				#endif
				printf("Migrating buffers from kernel\n");
				OCL_CHECK(err,
					err = queues[d].enqueueMigrateMemObjects( {buffer_out[d][k], buffer_hw_iter[d][k]}, 
					CL_MIGRATE_MEM_OBJECT_HOST, &task_events[d][k], &memory_write_events[d][k][0]));

				#if DETAILED_PERF_METRICS
				kernel_d2h_time.stop();
				#endif
			}
		}
		
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
				sprintf(FileName, "%sfpga_nkM%d_nk%d_i%d_d%d_k%d.txt", KP_OUT_FILE, NKM_GRID, NKGRID, i, d, k);
					cfile = fopen(FileName, "w");
					for(int i=0; i<NSTATES; i++){
						fprintf(cfile, "%.15lf \n", hw_out[d][k].kprime[i]);
					}
					fclose(cfile);

					sprintf(FileName, "%sfpga_nkM%d_nk%d_i%d_d%d_k%d.txt", KCROSS_OUT_FILE, NKM_GRID, NKGRID, i, d, k);
					cfile = fopen(FileName, "w");
					for(int i=0; i<N_AGENTS; i++){
						fprintf(cfile, "%.15lf \n", hw_out[d][k].kcross[i]);
					}
					fclose(cfile);

					sprintf(FileName, "%sfpga_nkM%d_nk%d_i%d_d%d_k%d.txt", COEFFS_OUT_FILE, NKM_GRID, NKGRID, i, d, k);
					cfile = fopen(FileName, "w"); 
					for(int i=0; i<NCOEFF; i++){
						fprintf(cfile, "%.15lf \n", hw_out[d][k].coeff[i]);
					}
					fclose(cfile);

					sprintf(FileName, "%sfpga_nkM%d_nk%d_i%d_d%d_k%d.txt", R2BG_OUT_FILE, NKM_GRID, NKGRID, i, d, k);
					cfile = fopen(FileName, "w"); 
					fprintf(cfile, "%.15lf\n", hw_out[d][k].r2[0]);
					fprintf(cfile, "%.15lf\n", hw_out[d][k].r2[1]);
					fclose(cfile);

					sprintf(FileName, "%sfpga_nkM%d_nk%d_i%d_d%d_k%d.txt", ITER_OUT_FILE, NKM_GRID, NKGRID, i, d, k);
					cfile = fopen(FileName, "w");
					total_egm_iter[d][k] = 0;
					for(int i=1; i<=hw_iter[d][k][0]; i++){
						fprintf(cfile, "%d \n", hw_iter[d][k][i]);
						total_egm_iter[d][k] += hw_iter[d][k][i];
					}
					fclose(cfile);

			}
		}

		#if DETAILED_PERF_METRICS
		write_results_time.stop();
		#endif

		for(int d=0; d<NUM_DEVICES; d++){
			for (int k = 0; k < NUM_KERNELS; k++) {
				printf("i=%d d=%d k=%d Bad Coeff 0: %.15lf\n", i, d, k, hw_out[d][k].coeff[0]);
				printf("i=%d d=%d k=%d Bad Coeff 1: %.15lf\n", i, d, k, hw_out[d][k].coeff[1]);
				printf("i=%d d=%d k=%d Bad R2: %.15lf\n", i, d, k, hw_out[d][k].r2[0]);
				printf("i=%d d=%d k=%d Good Coeff 0: %.15lf\n", i, d, k, hw_out[d][k].coeff[2]);
				printf("i=%d d=%d k=%d Good Coeff 1: %.15lf\n", i, d, k, hw_out[d][k].coeff[3]);
				printf("i=%d d=%d k=%d Good R2: %.15lf\n\n", i, d, k, hw_out[d][k].r2[1]);
				printf("i=%d d=%d k=%d Total EGM iter: %d\n", i, d, k, total_egm_iter[d][k]);
				printf("i=%d d=%d k=%d Total Main loop iter: %d\n\n", i, d, k, hw_iter[d][k][0]);
			}
		}

		for(int d=0; d<NUM_DEVICES; d++){
			for (int k = 0; k < NUM_KERNELS; k++) {
				free_all(&in[d][k]); 
			}
		}
	}

	#if DETAILED_PERF_METRICS
	kernel_total_time.stop();
	#endif
	
	// Performance metrics (main)
	#if DETAILED_PERF_METRICS
		total_time.stop();
	#endif

	real tot_time = total_time.latency() * double(1e-9);
	std::cout << "Total latency of the loop is: " << tot_time << "s." << std::endl;
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

	const char* num_of_devices;
	if(NUM_DEVICES==1)
		num_of_devices = "I";
	else if (NUM_DEVICES==2)
		num_of_devices = "II";
	else if (NUM_DEVICES==8)
		num_of_devices = "III";
	else
	{
	assert(false && "Invalid value for NUM_DEVICES");
	}

	FILE *cfile;
	char FileName[512];
	printf("\nWriting total kernel execution time to file\n");
	sprintf(FileName, "fpga%s-nKM%d-nk%d-time-tot.txt", num_of_devices, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", tot_time);
	fclose(cfile);
	
	sprintf(FileName, "fpga%s-nKM%d-nk%d-open-init-time.txt", num_of_devices, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", opencl_init_time.latency() * double(1e-9));
	fclose(cfile);

	sprintf(FileName, "fpga%s-nKM%d-nk%d-kernel-tot-time.txt", num_of_devices, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", kernel_total_time.latency() * double(1e-9));
	fclose(cfile);

	sprintf(FileName, "fpga%s-nKM%d-nk%d-init-time.txt", num_of_devices, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", kernel_init_time.latency() * double(1e-9));
	fclose(cfile);

	sprintf(FileName, "fpga%s-nKM%d-nk%d-kernel-time.txt", num_of_devices, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", kernel_flush_time.latency() * double(1e-9));
	fclose(cfile);

	sprintf(FileName, "fpga%s-nKM%d-nk%d-h2d-time.txt", num_of_devices, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", kernel_h2d_time.latency() * double(1e-9));
	fclose(cfile);

    sprintf(FileName, "fpga%s-nKM%d-nk%d-kernel-task-time.txt", num_of_devices, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", kernel_task_time.latency() * double(1e-9));
	fclose(cfile);
	
	sprintf(FileName, "fpga%s-nKM%d-nk%d-d2h-time.txt", num_of_devices, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", kernel_d2h_time.latency() * double(1e-9));
	fclose(cfile);
	
	sprintf(FileName, "fpga%s-nKM%d-nk%d-write-time.txt", num_of_devices, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", write_results_time.latency() * double(1e-9));
	fclose(cfile);
	
	return 0;
}

cl::Program load_cl2_binary(cl::Program::Binaries bins, cl::Device device, cl::Context context) {
    cl_int err;
    std::vector<cl::Device> devices(1, device);
    OCL_CHECK(err, cl::Program program(context, devices, bins, nullptr, &err));
    return program;
}
