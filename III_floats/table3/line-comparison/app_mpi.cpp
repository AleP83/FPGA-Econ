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