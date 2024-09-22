%% Step 0: Sanity Checks
clear
clc
close all
addpath('../IV_paper/results/');
addpath(genpath('../I_estimation_results/'));
addpath('./SanityChecks/');
addpath('./EEErrors/');

format long g
INPUTFOLDER='../I_estimation_results/';   % Results of model estimation in FPGA and CPU

nklist = [100 200 300];
nKMlist = [4 8];

%% f1.2xlarge: Single-kernel design checks
device_design = 'fpgaI_knl-1';
for i = 1:length(nklist)
    for j = 1:length(nKMlist)
        ngridk = nklist(i);
        ngridkm = nKMlist(j);
        SC(i,j).Diff_ = SCheck_fun(ngridk,ngridkm,INPUTFOLDER,device_design);
    end
end
disp('f1.2xlarge, Single-kernel Design: Sanity Checks: Done!')

%% f1.2xlarge: Three-kernel design checks
device_design = 'fpgaI';
ngridk = 100;
ngridkm = 4;
Diff_ = SCheck_fun(ngridk,ngridkm,INPUTFOLDER,device_design);
disp('f1.2xlarge, Three-kernel Design: Sanity Checks: Done!')

%% f1.4xlarge: Three-kernel design checks
device_design = 'fpgaII';
ngridk = 100;
ngridkm = 4;
Diff_ = SCheck_fun(ngridk,ngridkm,INPUTFOLDER,device_design);
disp('f1.4xlarge, Three-kernel Design: Sanity Checks: Done!')


%% f1.16xlarge: Three-kernel design checks
device_design = 'fpgaIII';
ngridk = 100;
ngridkm = 4;
Diff_ = SCheck_fun(ngridk,ngridkm,INPUTFOLDER,device_design);
disp('f1.16xlarge, Three-kernel Design: Sanity Checks: Done!')