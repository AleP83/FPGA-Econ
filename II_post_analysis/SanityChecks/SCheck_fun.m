function Diff_ = SCheck_fun(ngridk,ngridkm,inputdir,device_design)
%Sanity Check Function: Compare kprime, kcross, coeffs computed in different platforms
nA = 2;                %size(kprime_MMV,3);         % Aggregate shock's states
nepsilon = 2;        %size(kprime_MMV,4); % Idiosyncratic employment status shock's states 

%% Load Policy function
% FPGA
file_path = [inputdir,'kprime/kpo_',device_design,'_nkM',num2str(ngridkm)','_nk',num2str(ngridk),'_i0_d0_k0_of_1200.txt'];
kprime_FPGA = load_policy_function(file_path,ngridk,ngridkm,nA,nepsilon);
clearvars file_path
% C-CPU
file_path = [inputdir,'kprime/kpo_cpu_cores1_i0_of_1200_nKM',num2str(ngridkm)','_nk',num2str(ngridk),'.txt'];
kprime_CPU = load_policy_function(file_path,ngridk,ngridkm,nA,nepsilon);
clearvars file_path
% Matlab
file_path = [inputdir,'matlab/MMV/Solution_to_model_nKM',num2str(ngridkm),'-nk',num2str(ngridk),'.mat'];
load(file_path,'kprime');
kprime_MMV = kprime;
clearvars kprime 
clearvars file_path

%% Load Distribution
% FPGA
file_path = [inputdir,'kcross/kcross_',device_design,'_nkM',num2str(ngridkm)','_nk',num2str(ngridk),'_i0_d0_k0_of_1200.txt'];
fileID = fopen(file_path,'r');
kcross_FPGA = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST file_path
% C-CPU
file_path = [inputdir,'kcross/kcross_cpu_cores1_i0_of_1200_nKM',num2str(ngridkm)','_nk',num2str(ngridk),'.txt'];
fileID = fopen(file_path,'r');
kcross_CPU = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST file_path

% Matlab
file_path = [inputdir,'matlab/MMV/Solution_to_model_nKM',num2str(ngridkm),'-nk',num2str(ngridk),'.mat'];
load(file_path,'kcross');
kcross_MMV = kcross';
clearvars kcross 
clearvars file_path

%% Load Coefficients
% FPGA
file_path = [inputdir,'coefficients/coeffs_',device_design,'_nkM',num2str(ngridkm)','_nk',num2str(ngridk),'_i0_d0_k0_of_1200.txt'];
fileID = fopen(file_path,'r');
coeffs_FPGA = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST file_path
% C-CPU
file_path = [inputdir,'coefficients/coeffs_cpu_cores1_i0_of_1200_nKM',num2str(ngridkm)','_nk',num2str(ngridk),'.txt'];
fileID = fopen(file_path,'r');
coeffs_CPU  = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST file_path
% Matlab
file_path = [inputdir,'matlab/MMV/Solution_to_model_nKM',num2str(ngridkm),'-nk',num2str(ngridk),'.mat'];
load(file_path,'B');
coeffs_MMV = B';
clearvars B file_path 

%% C-CPU vs FPGA
Diff_.CPU_FPGA.kcross =  difference_aux(kcross_CPU,kcross_FPGA);
Diff_.CPU_FPGA.kprime = difference_aux(kprime_CPU,kprime_FPGA);
Diff_.CPU_FPGA.coeffs = difference_aux(coeffs_CPU,coeffs_FPGA);
%% C-CPU vs Matlab
Diff_.CPU_Matlab.kcross =  difference_aux(kcross_CPU,kcross_MMV);
Diff_.CPU_Matlab.kprime = difference_aux(kprime_CPU,kprime_MMV);
Diff_.CPU_Matlab.coeffs = difference_aux(coeffs_CPU,coeffs_MMV);

end