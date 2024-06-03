clear
clc
close all;
nklist = [100,200,300];
NKMlist = [4, 8];

% Generate results for different sizes of individual capital holdigns grid
OUTPUTFOLDER = '../../I_estimation_results/matlab/';
for i=1:length(nklist)
    for j=1:length(NKMlist)
        nk = nklist(i);
        NKM = NKMlist(j);
        execution = MMV_func(nk,NKM,OUTPUTFOLDER);
        assert(execution==1);
    end
end 

