#ifdef _OPENMPI_MODE
    #define OMPI_MODE			1       // 1 ON, 0 OFF
#elif _FPGA_MODE
    #define FPGA_MODE      		1       // 1 ON, 0 OFF
#elif _SERIAL_CPU_MODE
    #define SERIAL_CPU_MODE     1       // 1 ON, 0 OFF
#endif

#define ID_MASTER           0       // never touch it
#define N_MODEL             1200     // total number of models

#include "app.h"

using std::array;
using std::map;
using std::vector;

#if OMPI_MODE

int mpi_enabled;

// Functions
void parameters_range_pertask(int n_start,int n_end,int n_tasks,int id_task,int *i_min_task_id,int *i_max_task_id)
{
    int quotient;
    int reminder;
    int N_total;
    N_total        = n_end-n_start+1;
    quotient       = N_total / n_tasks;
    reminder       = N_total % n_tasks;
    
    if(reminder> id_task)
    {
        *i_min_task_id = n_start + id_task * quotient + id_task;
        *i_max_task_id = *i_min_task_id + quotient;
        
    }
    else
    {
        *i_min_task_id = n_start + id_task * quotient + reminder;
        *i_max_task_id = *i_min_task_id + quotient - 1;
    }
    
}


int main(){

	#if DETAILED_PERF_METRICS
	stopwatch total_time;
	stopwatch openmpi_init_time;
	stopwatch thread_init_time;
	stopwatch thread_compute_time;
	stopwatch single_core_time;
	stopwatch write_results_time;
	#endif

	#if DETAILED_PERF_METRICS
	total_time.start();
	openmpi_init_time.start();
	#endif
	//-----------------------------------------------Initialize Open MPI-----------------------------------------------
	//Initialize
    mpi_enabled = MPI_Init(NULL, NULL);

    if (mpi_enabled != MPI_SUCCESS) {
        printf ("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, mpi_enabled);
    }

	// Collect number of processes
    int n_tasks;
    MPI_Comm_size(MPI_COMM_WORLD, &n_tasks);

	// Get the id of the processes
    int id_task;
    MPI_Comm_rank(MPI_COMM_WORLD, &id_task);

    if (n_tasks > 1) {
        mpi_enabled = 1;
    }
    else {
        mpi_enabled = 0;
    }

    if (id_task==ID_MASTER) {
        // printf("\nThe Calibration is performed using %d processor(s).\n",n_tasks);    
    }

	// Sync all processes
	MPI_Barrier(MPI_COMM_WORLD);
		
	// Range of tasks per processor.
	int i_min_task_id, i_max_task_id; 

	// Define the Block to be assigned to each task
    parameters_range_pertask(0,N_MODEL-1,n_tasks,id_task,&i_min_task_id,&i_max_task_id);

	#if DETAILED_PERF_METRICS
	openmpi_init_time.stop();
	#endif
	//-----------------------------------------------Initialize Open MPI: END -----------------------------------------------
	#if DETAILED_PERF_METRICS
	single_core_time.start();
	#endif   

	for(int i = i_min_task_id; i <= i_max_task_id; i++) { 
		#if DETAILED_PERF_METRICS
		thread_init_time.start();
		#endif 
		preinit_t hw_preinit;
		out_t out;
		int hw_iter[300] = {0};

		env_t env;
		input_t in;
		vars_t vars;
		
		init_all(&env, &in, &vars);

		for(int i=0; i<NKGRID; i++){
			hw_preinit.k[i] = env.k[i];
		}

		for(int i=0; i<NKM_GRID; i++){
			hw_preinit.km[i] = env.km[i];
		}

		for(int i=0; i<NSTATES; i++){
			hw_preinit.wealth[i] = env.wealth[i];
		}

		for(int i=0; i<NSTATES; i++){
			hw_preinit.kprime[i] = vars.kprime_a[i];
		}

		#if DETAILED_PERF_METRICS
		thread_init_time.stop();
		#endif 

		#if DETAILED_PERF_METRICS
		thread_compute_time.start();
		#endif 

		runOnfpga(in.agshock, in.idshock, &hw_preinit, &out, hw_iter);

		#if DETAILED_PERF_METRICS
		thread_compute_time.stop();
		#endif 

		#if DETAILED_PERF_METRICS
		write_results_time.start();
		#endif 

		FILE *cfile;
		char FileName[512];
		printf("Migrating buffers from kernel\n");
		sprintf(FileName, "%scpu_cores%d_i%d_of_%d_nKM%d_nk%d.txt", OPENMPI_KP_OUT_FILE, n_tasks, i, N_MODEL, NKM_GRID, NKGRID);
		cfile = fopen(FileName, "w");
		for(int i=0; i<NSTATES; i++){
			fprintf(cfile, "%.15lf \n", out.kprime[i]);
		}
		fclose(cfile);
		
		sprintf(FileName, "%scpu_cores%d_i%d_of_%d_nKM%d_nk%d.txt", OPENMPI_KCROSS_OUT_FILE, n_tasks, i, N_MODEL, NKM_GRID, NKGRID);
		cfile = fopen(FileName, "w"); 
		for(int i=0; i<N_AGENTS; i++){
			fprintf(cfile, "%.15lf \n", out.kcross[i]);
		}
		fclose(cfile);

		sprintf(FileName, "%scpu_cores%d_i%d_of_%d_nKM%d_nk%d.txt", OPENMPI_COEFFS_OUT_FILE, n_tasks, i, N_MODEL, NKM_GRID, NKGRID);
		cfile = fopen(FileName, "w"); 
		for(int i=0; i<NCOEFF; i++){
			fprintf(cfile, "%.15lf \n", out.coeff[i]);
		}
		fclose(cfile);

		sprintf(FileName, "%scpu_cores%d_i%d_of_%d_nKM%d_nk%d.txt", OPENMPI_R2BG_OUT_FILE, n_tasks, i, N_MODEL, NKM_GRID, NKGRID);
		cfile = fopen(FileName, "w"); 
		fprintf(cfile, "%.15lf\n", out.r2[0]);
		fprintf(cfile, "%.15lf\n", out.r2[1]);
		fclose(cfile);

		int total_egm_iter = 0;
		sprintf(FileName, "%scpu_cores%d_i%d_of_%d_nKM%d_nk%d.txt", OPENMPI_ITER_OUT_FILE, n_tasks, i, N_MODEL, NKM_GRID, NKGRID);
		cfile = fopen(FileName, "w"); 
		for(int i=1; i<=hw_iter[0]; i++){
			fprintf(cfile, "%d\n", hw_iter[i]);
			total_egm_iter += hw_iter[i];
		}
		fclose(cfile);

		printf("Total EGM iter: %d\n", total_egm_iter);
		printf("Total Main loop iter: %d\n", hw_iter[0]); 
		printf("Bad Coeff 0: %.15lf\n", out.coeff[0]);
		printf("Bad Coeff 1: %.15lf\n", out.coeff[1]);
		printf("Good Coeff 0: %.15lf\n", out.coeff[2]);
		printf("Good Coeff 1: %.15lf\n", out.coeff[3]);
		printf("Bad R2: %.15lf\n", out.r2[0]);
		printf("Good R2: %.15lf\n\n", out.r2[1]);

		#if DETAILED_PERF_METRICS
		write_results_time.stop();
		#endif 
		
		// Deallocate Memory
		free_all(&in);
	}

	#if DETAILED_PERF_METRICS
	single_core_time.stop();
	#endif 
   
   //-----------------------------------------------Finalize Open MPI-----------------------------------------------
	#if OMPI_MODE
	// Finalize MPI environment
		MPI_Finalize();
	#endif

	#if DETAILED_PERF_METRICS
	total_time.stop();
	#endif

	double tot_time = total_time.latency() * double(1e-9);
	//-----------------------------------------------Save results-----------------------------------------------
	std::cout << "Total latency of the loop is: " << tot_time << "s." << std::endl;
	std::cout << "Total latency of openmpi initialization is: " << openmpi_init_time.latency() * double(1e-9) << "s." << std::endl;
	std::cout << "Total thread init time is: " << thread_init_time.latency()*double(1e-9) << "s." << std::endl;
	std::cout << "Total thread compute time is: " << thread_compute_time.latency()*double(1e-9) << "s." << std::endl;
	std::cout << "Total single core time is: " << single_core_time.latency()*double(1e-9) << "s." << std::endl;
	std::cout << "Total time to write results is: " << write_results_time.latency()*double(1e-9) << "s." << std::endl;
	std::cout << "---------------------------------------------------------------" << std::endl;
	std::cout << "Average latency of each kernel is: " <<(thread_init_time.avg_latency() + thread_compute_time.avg_latency() + write_results_time.avg_latency()) * double(1e-9) << "s." << std::endl;

	FILE *cfile;
	char FileName[512];
	sprintf(FileName, "cpu-cores%d-nKM%d-nk%d-time-tot.txt", n_tasks, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", tot_time);
	fclose(cfile);

	sprintf(FileName, "cpu-cores%d-nKM%d-nk%d-kernel-time.txt", n_tasks, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", thread_compute_time.latency()*double(1e-9));
	fclose(cfile);

	sprintf(FileName, "cpu-cores%d-nKM%d-nk%d-init-time.txt", n_tasks, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", thread_init_time.latency()*double(1e-9));
	fclose(cfile);

	sprintf(FileName, "cpu-cores%d-nKM%d-nk%d-open-init-time.txt", n_tasks, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", openmpi_init_time.latency()*double(1e-9));
	fclose(cfile);

	sprintf(FileName, "cpu-cores%d-nKM%d-nk%d-single-core-time.txt", n_tasks, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", single_core_time.latency()*double(1e-9));
	fclose(cfile);

	sprintf(FileName, "cpu-cores%d-nKM%d-nk%d-write-time.txt", n_tasks, NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", write_results_time.latency()*double(1e-9));
	fclose(cfile);

	return 0;

}

#elif SERIAL_CPU_MODE

int main(){
	#if DETAILED_PERF_METRICS
	stopwatch total_time;
	stopwatch thread_init_time;
	stopwatch thread_compute_time;
	stopwatch write_results_time;
	#endif

	#if DETAILED_PERF_METRICS
	total_time.start();
	#endif

    for(int i = 0; i < N_MODEL; i++) { 
		#if DETAILED_PERF_METRICS
		thread_init_time.start();
		#endif 
		preinit_t hw_preinit;
		out_t out;
		int hw_iter[300] = {0};

		env_t env;
		input_t in;
		vars_t vars;

		init_all(&env, &in, &vars);

		for(int i=0; i<NKGRID; i++){
			hw_preinit.k[i] = env.k[i];
		}

		for(int i=0; i<NKM_GRID; i++){
			hw_preinit.km[i] = env.km[i];
		}

		for(int i=0; i<NSTATES; i++){
			hw_preinit.wealth[i] = env.wealth[i];
		}

		for(int i=0; i<NSTATES; i++){
			hw_preinit.kprime[i] = vars.kprime_a[i];
		}

		#if DETAILED_PERF_METRICS
		thread_init_time.stop();
		#endif

		#if DETAILED_PERF_METRICS
		thread_compute_time.start();
		#endif 

		runOnfpga(in.agshock, in.idshock, &hw_preinit, &out, hw_iter);

		#if DETAILED_PERF_METRICS
		thread_compute_time.stop();
		#endif 

		#if DETAILED_PERF_METRICS
		write_results_time.start();
		#endif 

		FILE *cfile;
		char FileName[512];
		printf("Migrating buffers from kernel\n");
		sprintf(FileName, "%scpu_cores1_i%d_of_%d_nKM%d_nk%d.txt", CPU_KP_OUT_FILE, i, N_MODEL, NKM_GRID, NKGRID);
		cfile = fopen(FileName, "w");
		for(int i=0; i<NSTATES; i++){
			fprintf(cfile, "%.15lf \n", out.kprime[i]);
		}
		fclose(cfile);

		sprintf(FileName, "%scpu_cores1_i%d_of_%d_nKM%d_nk%d.txt", CPU_KCROSS_OUT_FILE, i, N_MODEL, NKM_GRID, NKGRID);
		cfile = fopen(FileName, "w"); 
		for(int i=0; i<N_AGENTS; i++){
			fprintf(cfile, "%.15lf \n", out.kcross[i]);
		}
		fclose(cfile);

		sprintf(FileName, "%scpu_cores1_i%d_of_%d_nKM%d_nk%d.txt", CPU_COEFFS_OUT_FILE, i, N_MODEL, NKM_GRID, NKGRID);
		cfile = fopen(FileName, "w"); 
		for(int i=0; i<NCOEFF; i++){
			fprintf(cfile, "%.15lf \n", out.coeff[i]);
		}
		fclose(cfile);

		sprintf(FileName, "%scpu_cores1_i%d_of_%d_nKM%d_nk%d.txt", CPU_R2BG_OUT_FILE, i, N_MODEL, NKM_GRID, NKGRID);
		cfile = fopen(FileName, "w"); 
		fprintf(cfile, "%.15lf\n", out.r2[0]);
		fprintf(cfile, "%.15lf\n", out.r2[1]);
		fclose(cfile);

		int total_egm_iter = 0;
		sprintf(FileName, "%scpu_cores1_i%d_of_%d_nKM%d_nk%d.txt", CPU_ITER_OUT_FILE, i, N_MODEL, NKM_GRID, NKGRID);
		cfile = fopen(FileName, "w"); 
		for(int i=1; i<=hw_iter[0]; i++){
			fprintf(cfile, "%d\n", hw_iter[i]);
			total_egm_iter += hw_iter[i];
		}
		fclose(cfile);

		printf("Total EGM iter: %d\n", total_egm_iter);
		printf("Total Main loop iter: %d\n", hw_iter[0]); 
		printf("Bad Coeff 0: %.15lf\n", out.coeff[0]);
		printf("Bad Coeff 1: %.15lf\n", out.coeff[1]);
		printf("Good Coeff 0: %.15lf\n", out.coeff[2]);
		printf("Good Coeff 1: %.15lf\n", out.coeff[3]);
		printf("Bad R2: %.15lf\n", out.r2[0]);
		printf("Good R2: %.15lf\n\n", out.r2[1]);

		#if DETAILED_PERF_METRICS
		write_results_time.stop();
		#endif 
        
		// Deallocate Memory
		free_all(&in);
    }
   
	#if DETAILED_PERF_METRICS
	total_time.stop();
	#endif

	double tot_time = total_time.latency() * double(1e-9);
	//-----------------------------------------------Save results-----------------------------------------------
	std::cout << "Total latency of the loop is: " << tot_time << "s." << std::endl;
	std::cout << "Total thread init time is: " << thread_init_time.latency()*double(1e-9) << "s." << std::endl;
	std::cout << "Total thread compute time is: " << thread_compute_time.latency()*double(1e-9) << "s." << std::endl;
	std::cout << "Total time to write results is: " << write_results_time.latency()*double(1e-9) << "s." << std::endl;
	std::cout << "---------------------------------------------------------------" << std::endl;
	std::cout << "Average latency is: " <<(thread_init_time.avg_latency() + thread_compute_time.avg_latency() + write_results_time.avg_latency()) * double(1e-9) << "s." << std::endl;

	FILE *cfile;
	char FileName[512];
	sprintf(FileName, "cpu-cores1-nKM%d-nk%d-time-tot.txt", NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", tot_time);
	fclose(cfile);

	sprintf(FileName, "cpu-cores1-nKM%d-nk%d-kernel-time.txt", NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", thread_compute_time.latency()*double(1e-9));
	fclose(cfile);

	sprintf(FileName, "cpu-cores1-nKM%d-nk%d-init-time.txt", NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", thread_init_time.latency()*double(1e-9));
	fclose(cfile);

	sprintf(FileName, "cpu-cores1-nKM%d-nk%d-write-time.txt", NKM_GRID, NKGRID);
	cfile = fopen(FileName, "w");
	fprintf(cfile, "%.15lf \n", write_results_time.latency()*double(1e-9));
	fclose(cfile);


	return 0;
}

#elif FPGA_MODE

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
	//aligned_allocator<int> makes sure that the alignment of our vector is along integer boundaries. 
	//(The list doesn’t start in the middle of an integer boundary such as on the address 0x1
	// vector<vector<out_t, 		aligned_allocator<real>> >  hw_out(NUM_DEVICES, vector<out_t,		aligned_allocator<real>> (NUM_KERNELS));
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

	// auto fileBuf = xcl::read_binary_file(binaryFile);
	// cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};

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
		// programs[d] = load_cl2_binary(bins, devices[d], contexts[d]);
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
			// Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and Device-to-host communication
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

				// for(int i=0; i<NKGRID; i++){
				// 	hw_preinit[d][k].k[i] = env[d][k].k[i];
				// }

				// for(int i=0; i<NKM_GRID; i++){
				// 	hw_preinit[d][k].km[i] = env[d][k].km[i];
				// }

				memcpy(agshock_ptr[d][k], in[d][k].agshock, AGSHOCK_ARR_SIZE);
				memcpy(idshock_ptr[d][k], in[d][k].idshock, IDSHOCK_ARR_SIZE);

				#if DETAILED_PERF_METRICS
				kernel_init_time.stop();
				#endif

				// ------------------------------------------------------
				// Step 2: Copy Input data from Host to Global Memory on the device
				// ------------------------------------------------------
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

		// --------------------------------------------------
		// Step 3: Release Allocated Resources
		// --------------------------------------------------
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

#endif
