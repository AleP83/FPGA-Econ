function [Table_EE_FPGA,Table_EE_CPU,Table_relative_EE] = EEE_fun(ngridk,ngridkm,inputdir,outputdir)
%Euler Equation Error Functin. The function receives as inputs the grid sizes and file path and returns the EEE.
%{
ngridk=200;         % number of grid points on individual capital holdings
ngridkm=4;          % number of grid points for km
inputdir:               directory with results of matlab estimation used to
outputdir:             output directory
%}
nA = 2;                %size(kprime_MMV,3);         % Aggregate shock's states
nepsilon = 2;        %size(kprime_MMV,4); % Idiosyncratic employment status shock's states 

%% Compute Euler Equation Errors FPGA
display(['Compute Euler Equation Errors FPGA: (',num2str(ngridk),',',num2str(ngridkm),')'])
tic
% Load Policy function
file_path = [inputdir,'kprime/kpo_fpgaI_knl-1_nkM',num2str(ngridkm)','_nk',num2str(ngridk),'_i0_d0_k0_of_1200.txt'];
kprime_FPGA = load_policy_function(file_path,ngridk,ngridkm,nA,nepsilon);
% Compute Euler Equation Errors 
Table_EE_FPGA= EE_MMV(kprime_FPGA,inputdir,ngridk,ngridkm);
clearvars file_path
toc
%% Compute Euler Equation Errors CPU
display(['Compute Euler Equation Errors CPU: (',num2str(ngridk),',',num2str(ngridkm),')'])
tic
% Load Policy function
file_path = [inputdir,'kprime/kpo_cpu_cores1_i0_of_1200_nKM',num2str(ngridkm)','_nk',num2str(ngridk),'.txt'];
kprime_CPU = load_policy_function(file_path,ngridk,ngridkm,nA,nepsilon);
% Compute Euler Equation Errors 
Table_EE_CPU= EE_MMV(kprime_CPU,inputdir,ngridk,ngridkm);
clearvars file_path
toc
%% Compute Euler Equation Errors Matlab
display(['Compute Euler Equation Errors Matlab: (',num2str(ngridk),',',num2str(ngridkm),')'])
tic
file_path = [inputdir,'matlab/MMV/Solution_to_model_nKM',num2str(ngridkm),'-nk',num2str(ngridk),'.mat'];
load(file_path,'kprime');
kprime_MMV = kprime;
clearvars kprime 
% Compute Euler Equation Errors 
Table_EE_Matlab= EE_MMV(kprime_MMV,inputdir,ngridk,ngridkm);
clearvars file_path
toc
%% Relative Euler Equation Errors 
display('------------------------------------------------------------------------')
display('--------------------------- RESULTS ---------------------------------')
display('------------------------------------------------------------------------')
Table_relative_EE(1,1)= {'average'};Table_relative_EE(1,2)={abs((Table_EE_FPGA{1,2}-Table_EE_CPU{1,2})/Table_EE_CPU{1,2})*100};
Table_relative_EE(2,1)= {'maximal'};Table_relative_EE(2,2)={abs((Table_EE_FPGA{2,2}-Table_EE_CPU{2,2})/Table_EE_CPU{2,2})*100};

disp('Table 1.  Euler equation errors FPGA'); Table_EE_FPGA
disp('Table 1.  Euler equation errors CPU'); Table_EE_CPU
disp('Table 1.  Euler equation errors Matlab'); Table_EE_Matlab
disp('Table 1.  Relative Euler equation errors FPGA vs CPU'); Table_relative_EE

%% Sanity Check: Comparing Policy Functions:  (MMV (2010) - linear interpolation) vis-a-vis C Results (FPGA and CPU) 
% Compare FPGA with Matlab Results
for i=1:4
    assert(size(kprime_FPGA,i)==size(kprime_MMV,i)); 
    assert(size(kprime_CPU,i)==size(kprime_MMV,i)); 
end 
 
abs_rel_diff = abs((kprime_FPGA - kprime_MMV) ./ kprime_MMV)*100;
max_rel_diff_FPGA = max(abs_rel_diff(:));
clearvars abs_rel_diff

abs_rel_diff = abs((kprime_CPU - kprime_MMV) ./ kprime_MMV)*100;
max_rel_diff_CPU= max(abs_rel_diff(:));
clearvars abs_rel_diff

Table_max_rel_diff(1,1)= {'FPGA_matlab'};Table_max_rel_diff(1,2)={max_rel_diff_FPGA};
Table_max_rel_diff(2,1)= {'CPU_matlab'};Table_max_rel_diff(2,2)={max_rel_diff_CPU};

disp('Table 1.  Max relative difference FPGA/CPU-C vs Matlab'); Table_max_rel_diff

save([outputdir,'/EE_errors_results_nKM',num2str(ngridkm),'-nk',num2str(ngridk),'.mat']);


outputArg1 =1;

end