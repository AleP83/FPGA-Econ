%% Post-Analysis: uses estimation results in ../I_estimation_results/ to generate tables and results in the paper
clear
clc
close all
addpath('../IV_paper/results/');
addpath(genpath('../I_estimation_results/'));
addpath('./EEErrors/');

format long g
INPUTFOLDER='../I_estimation_results/';   % Results of model estimation in FPGA and CPU
OUTPUTFOLDER='../IV_paper/results/';        % Create inputs for paper
TABLEFOLDER='../III_floats/';                      % Organize results in tables
%% Declaration: devices, cores, power, carbon footprint, models
tot_economies = 1200;
tot_economies_120 = 120;
% Cores
cpu_cores = [1,8,48];cpu_corelist = {'1','8','48'};
fpga_cores = [1,2,8];fpga_corelist = {'I','II','III'};
% Prices
cpu_price=[0.119,0.952,5.712];
fpga_price=[1.65,3.3,13.2];
% Cost Savings
one_million_economies = 1000000;
% Resources in VU9P
Resources_.BRAM_VU9P=1680;
Resources_.DSP_VU9P=5640;
Resources_.REGISTERS_VU9P=1790400;
Resources_.URAM_VU9P=800;
Resources_.LUT_VU9P=895000;
% Energy
power_cpu = 8;      %(2/96)*384
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                   MANUALLY INPUT HERE
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Three-kernel Design: Avg power FPGA in Watts
% Source: ./I_estimation_results/power/power_fpgaI_nKM4_nk100_i0_d0_k0_of_1200.txt
power_fpga = 33;                        % peak: 37
% Single-kernel Design: Avg power FPGA in Watts
% Source:./I_estimation_results/power/power_fpgaI_knl-1_nKM4_nk100_i0_d0_k0_of_1200.txt
power_fpga_single_kernel = 17 ;    % peak: 21
%%%%%%% RESOURCES: Obtained from Vitis Analyzer
% Resources baseline
Resources_.baselineBRAM=101/Resources_.BRAM_VU9P*100;
Resources_.baselineDSP=437/Resources_.DSP_VU9P*100;
Resources_.baselineRegisters=71480/Resources_.REGISTERS_VU9P*100;
Resources_.baselineURAM=44/Resources_.URAM_VU9P*100;
Resources_.baselineLUTs=53373/Resources_.LUT_VU9P*100;
% Resources pipeline
Resources_.pipelinenBRAM=120/Resources_.BRAM_VU9P*100;
Resources_.pipelineDSP=546/Resources_.DSP_VU9P*100;
Resources_.pipelineRegisters=91731/Resources_.REGISTERS_VU9P*100;
Resources_.pipelineURAM=44/Resources_.URAM_VU9P*100;
Resources_.pipelineLUTs=82306/Resources_.LUT_VU9P*100;
% Resources three-kernel 100-4
Resources_.acrossdataparallelBRAM=248/Resources_.BRAM_VU9P*100*3;
Resources_.acrossdataparallelDSP=1040/Resources_.DSP_VU9P*100*3;
Resources_.acrossdataparallelRegisters=153464/Resources_.REGISTERS_VU9P*100*3;
Resources_.acrossdataparallelURAM=44/Resources_.URAM_VU9P*100*3;
Resources_.acrossdataparallelLUTs=170150/Resources_.LUT_VU9P*100*3;
% Resources single-kernel 100-4
Resources_.withindataparallelBRAM=358/Resources_.BRAM_VU9P*100;
Resources_.withindataparallelDSP=1756/Resources_.DSP_VU9P*100;
Resources_.withindataparallelRegisters=214849/Resources_.REGISTERS_VU9P*100;
Resources_.withindataparallelURAM=43/Resources_.URAM_VU9P*100;
Resources_.withindataparallelLUTs=225608/Resources_.LUT_VU9P*100;
% Resources single-kernel 200-4
Resources_.bramnKMIkII=459/Resources_.BRAM_VU9P*100;
Resources_.dspnKMIkII=1756/Resources_.DSP_VU9P*100;
Resources_.registernKMIkII=214840/Resources_.REGISTERS_VU9P*100;
Resources_.uramnKMIkII=43/Resources_.URAM_VU9P*100;
Resources_.lutnKMIkII=232396/Resources_.LUT_VU9P*100;
% Resources single-kernel 300-4
Resources_.bramnKMIkIII=556/Resources_.BRAM_VU9P*100;
Resources_.dspnKMIkIII=1756/Resources_.DSP_VU9P*100;
Resources_.registernKMIkIII=216993/Resources_.REGISTERS_VU9P*100;
Resources_.uramnKMIkIII=43/Resources_.URAM_VU9P*100;
Resources_.lutnKMIkIII=237672/Resources_.LUT_VU9P*100;
% Resources single-kernel 100-8
Resources_.bramnKMIIkI=459/Resources_.BRAM_VU9P*100;
Resources_.dspnKMIIkI=1766/Resources_.DSP_VU9P*100;
Resources_.registernKMIIkI=215882/Resources_.REGISTERS_VU9P*100;
Resources_.uramnKMIIkI=43/Resources_.URAM_VU9P*100;
Resources_.lutnKMIIkI=227620/Resources_.LUT_VU9P*100;
% Resources single-kernel 200-8
Resources_.bramnKMIIkII=637/Resources_.BRAM_VU9P*100;
Resources_.dspnKMIIkII=1766/Resources_.DSP_VU9P*100;
Resources_.registernKMIIkII=217907/Resources_.REGISTERS_VU9P*100;
Resources_.uramnKMIIkII=43/Resources_.URAM_VU9P*100;
Resources_.lutnKMIIkII=234342/Resources_.LUT_VU9P*100;
% Resources single-kernel 300-8
Resources_.bramnKMIIkIII=794/Resources_.BRAM_VU9P*100;
Resources_.dspnKMIIkIII=1766/Resources_.DSP_VU9P*100;
Resources_.registernKMIIkIII=219545/Resources_.REGISTERS_VU9P*100;
Resources_.uramnKMIIkIII=43/Resources_.URAM_VU9P*100;
Resources_.lutnKMIIkIII=239295/Resources_.LUT_VU9P*100;

% Print Resources
Resources_varlist = fieldnames(Resources_);
% Loop through each field name
for i = 1:length(Resources_varlist)
    fieldString = Resources_varlist{i};  % Get the field name
    fieldValue = Resources_.(fieldString);  % Access the field value
    filename = strcat(OUTPUTFOLDER,'resources-',fieldString,'.txt');
    fileID = fopen(filename,'w');
    fprintf(fileID,'%4.2f',round(fieldValue,2));
    ST = fclose(fileID);assert(ST==0)
    clearvars filename fileID ST fieldString fieldValue
end
% Performance CPU/FPGA
Performance_.cpu_sim_sh=0.72;                   % ../I_estimation_results/reports/gprof/cpu_analysis.txt
Performance_.cpu_ihp_sh=0.28;                    % ../I_estimation_results/reports/gprof/cpu_analysis.txt
Performance_.fpga_sim=7.137 * 52;              % milliseconds ../I_estimation_results/reports/1ker_100k_4km_runOnfpga_csynth.rpt and log_fpgaI_knl-1_nKM4_nk100_1200.txt
Performance_.fpga_ihp=10.588*36019*1e-3; % milliseconds  ../I_estimation_results/reports/1ker_100k_4km_runOnfpga_csynth.rpt log_fpgaI_knl-1_nKM4_nk100_1200.txt

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                   END MANUALLY INPUT HERE
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Carbon Footprint
summit_power=13; % W
w_gas = 0.37;
w_coal = 0.26;
w_ren = 0.37;
weights = w_gas +w_coal + w_ren;
CO2_per_kWh_gas = 0.91;
CO2_per_kWh_coal = 2.21;
CO2_per_kWh_ren = 0.1;
assert(weights==1)
Tot_core_hour_summit = 150000000;
metric_tons_CO2_per_car = 5;        % Estimates Andrew Monaghan, CU Boulder
% -------------------------------------------------------
devicelist = {'cpu-cores','fpga'};
KMlist = {'4','8'};                                        % grid size of aggregate capital
klist = {'100','200','300'};                            % grid size of individual capital holding
devicelistcpu = {'cpu-cores1','cpu-cores8','cpu-cores48'};
devicelistfpga = {'fpgaI','fpgaII','fpgaIII'};
% -------------------------------------------------------

%% Step 1.  Benchmark Model. Create matrix(device,cores) with: time, cost, energysavings
NKM= '4';
Nk= '100';
for id=1:length(devicelist)

    % device
    chip = devicelist{id};
    switch chip
        case 'cpu-cores'

            corelist = cpu_corelist; %{'1','8','48'};

        case 'fpga'
            corelist = fpga_corelist; %{'I','II','III'};

        otherwise 
            error('No option selected');
    end

    % loop over cores
    for inumberofchips=1:length(corelist)

        % device-cores
        device = strcat(chip,corelist{inumberofchips});

        %%%%%%%%%%%%%%%%%%%%%%%
        % EXECUTION (TOTAL) TIME: exectime
        filename = strcat(INPUTFOLDER,'time/',device,'-nKM',NKM,'-nk',Nk,'-time-tot.txt');
        fileID = fopen(filename,'r');
        assert(fileID~=-1);
        exectime(id,inumberofchips) = fscanf(fileID,'%f');
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
        % Store .txt for paper
        filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-time-tot.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',exectime(id,inumberofchips));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
        %%%%%%%%%%%%%%%%%%%%%%%
        % INITIALIZATION TIME: inittime
        filename = strcat(INPUTFOLDER,'time/',device,'-nKM',NKM,'-nk',Nk,'-init-time.txt');
        fileID = fopen(filename,'r');
        assert(fileID~=-1);
        inittime(id,inumberofchips) = fscanf(fileID,'%f');
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
        % Store .txt for paper
        filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-init-time.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',inittime(id,inumberofchips));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
        %%%%%%%%%%%%%%%%%%%%%%%
        % WRITING TIME: writetime
        filename = strcat(INPUTFOLDER,'time/',device,'-nKM',NKM,'-nk',Nk,'-write-time.txt');
        fileID = fopen(filename,'r');
        assert(fileID~=-1);
        writetime(id,inumberofchips) = fscanf(fileID,'%f');
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
        % Store .txt for paper
        filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-write-time.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',writetime(id,inumberofchips));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
        %%%%%%%%%%%%%%%%%%%%%%%
        % KERNEL TIME: time
        filename = strcat(INPUTFOLDER,'time/',device,'-nKM',NKM,'-nk',Nk,'-kernel-time.txt');
        fileID = fopen(filename,'r');
        assert(fileID~=-1);
        time(id,inumberofchips) = fscanf(fileID,'%f');
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
        % Store .txt for paper
        filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-kernel-time.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',time(id,inumberofchips));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
        %%%%%%%%%%%%%%%%%%%%%%%
        % COST
        switch device
            case 'cpu-cores1'
                price = cpu_price(1); %0.119;
            case 'cpu-cores8'
                price = cpu_price(2); %0.952;
            case 'cpu-cores48'
                price = cpu_price(3); %5.712;
            case 'fpgaI'
                price = fpga_price(1); %1.65;
            case 'fpgaII'
                price = fpga_price(2); %3.30;
            case 'fpgaIII'
                price = fpga_price(3); %13.2;
            otherwise 
                error('No option selected');
        end
        cost(id,inumberofchips) = time(id,inumberofchips)/ 3600 * price;
        % Store .txt for paper
        filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-cost.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',cost(id,inumberofchips));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
        %%%%%%%%%%%%%%%%%%%%%%%
        % ENERGY
        switch device
            case 'cpu-cores1'
                watts = power_cpu *cpu_cores(1); %8;  %(2/96)*384
            case 'cpu-cores8'
                watts = power_cpu*cpu_cores(2); % 64;  %(16/96)*384
            case 'cpu-cores48'
                watts = power_cpu*cpu_cores(3); % 384;    %TDP for processor
            case 'fpgaI'
                watts = power_fpga*fpga_cores(1); %33;
            case 'fpgaII'
                watts = power_fpga*fpga_cores(2); %66;
            case 'fpgaIII'
                watts = power_fpga*fpga_cores(3); %264;
            otherwise 
                error('No option selected');
        end
        energy(id,inumberofchips) = time(id,inumberofchips) * watts;
        % Store .txt for paper
        filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-energy.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',energy(id,inumberofchips));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
    end

end

%% Step 2. Create matrix(cpu-devices,fpga-devices) with: speedup, cost savings, energy savings (NKM=4,Nk=100)
% Compute Relative speedup fpga vs cpu
for icpu=1:length(devicelistcpu)    %cpu-devices list
    for ifpga = 1:length(devicelistfpga)    % fpga-devices list
        
        % Speedup
        speedup(icpu,ifpga) = (time(1,icpu)/time(2,ifpga));
        filename = strcat(OUTPUTFOLDER,devicelistcpu{icpu},'-',devicelistfpga{ifpga},'-nKM',NKM,'-nk',Nk,'-speedup','.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',speedup(icpu,ifpga));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST

        % Costs Savings
        costsavings(icpu,ifpga) = cost(2,ifpga)/cost(1,icpu)*100;
        filename = strcat(OUTPUTFOLDER,devicelistcpu{icpu},'-',devicelistfpga{ifpga},'-nKM',NKM,'-nk',Nk,'-costsavings','.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',costsavings(icpu,ifpga));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST

        %Energy Savings
        energysavings(icpu,ifpga) = energy(2,ifpga)/energy(1,icpu)*100;
        filename = strcat(OUTPUTFOLDER,devicelistcpu{icpu},'-',devicelistfpga{ifpga},'-nKM',NKM,'-nk',Nk,'-energysavings','.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',energysavings(icpu,ifpga));
        %fprintf(fileID,'%i',1); %temporary until we get correct energy measures
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
    
    end
end


%% Step 3: Create table for Single-kernel (device,NKM,Nk) across
for iKM=1:length(KMlist)
    NKM = KMlist{iKM};
    for ik=1:length(klist)
        Nk = klist{ik};
        for id=1:length(devicelist)
            % device
            chip = devicelist{id};
            switch chip
                case 'cpu-cores'

                    corelist = cpu_corelist{1}; %{'1','8','48'};

                case 'fpga'
                    corelist = strcat(fpga_corelist{1},'-knl-1'); %{'I','II','III'};
                otherwise 
                error('No option selected');
            end
            
            % device-cores
            device = strcat(chip,corelist);
            %%%%%%%%%%%%%%%%%%%%%%%
            % EXECUTION (TOTAL) TIME: exectime
            filename = strcat(INPUTFOLDER,'time/',device,'-nKM',NKM,'-nk',Nk,'-time-tot.txt');
            fileID = fopen(filename,'r');
            assert(fileID~=-1);
            exectime_1ker(id,iKM,ik) = fscanf(fileID,'%f');
            ST = fclose(fileID);assert(ST==0)
            clearvars filename fileID ST
            % Store .txt for paper
            filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-time-tot.txt');
            fileID = fopen(filename,'w');
            fprintf(fileID,'%4.2f',exectime_1ker(id,iKM,ik));
            ST = fclose(fileID);assert(ST==0)
            clearvars filename fileID ST
            %%%%%%%%%%%%%%%%%%%%%%%
            % INITIALIZATION TIME: inittime
            filename = strcat(INPUTFOLDER,'time/',device,'-nKM',NKM,'-nk',Nk,'-init-time.txt');
            fileID = fopen(filename,'r');
            assert(fileID~=-1);
            inittime_1ker(id,iKM,ik) = fscanf(fileID,'%f');
            ST = fclose(fileID);assert(ST==0)
            clearvars filename fileID ST
            % Store .txt for paper
            filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-init-time.txt');
            fileID = fopen(filename,'w');
            fprintf(fileID,'%4.2f',inittime_1ker(id,iKM,ik));
            ST = fclose(fileID);assert(ST==0)
            clearvars filename fileID ST
            %%%%%%%%%%%%%%%%%%%%%%%
            % WRITING TIME: writetime
            filename = strcat(INPUTFOLDER,'time/',device,'-nKM',NKM,'-nk',Nk,'-write-time.txt');
            fileID = fopen(filename,'r');
            assert(fileID~=-1);
            writetime_1ker(id,iKM,ik) = fscanf(fileID,'%f');
            ST = fclose(fileID);assert(ST==0)
            clearvars filename fileID ST
            % Store .txt for paper
            filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-write-time.txt');
            fileID = fopen(filename,'w');
            fprintf(fileID,'%4.2f',writetime_1ker(id,iKM,ik));
            ST = fclose(fileID);assert(ST==0)
            clearvars filename fileID ST
            %%%%%%%%%%%%%%%%%%%%%%%
            % KERNEL TIME: time
            filename = strcat(INPUTFOLDER,'time/',device,'-nKM',NKM,'-nk',Nk,'-kernel-time.txt');
            fileID = fopen(filename,'r');
            assert(fileID~=-1);
            time_1ker(id,iKM,ik) = fscanf(fileID,'%f');
            ST = fclose(fileID);assert(ST==0)
            clearvars filename fileID ST
            % Store .txt for paper
            filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-kernel-time.txt');
            fileID = fopen(filename,'w');
            fprintf(fileID,'%4.2f',time_1ker(id,iKM,ik));
            ST = fclose(fileID);assert(ST==0)
            clearvars filename fileID ST
            %%%%%%%%%%%%%%%%%%%%%%%
            % cost_1ker
            switch device
                case 'cpu-cores1'
                    price = cpu_price(1); %0.119;
                case 'fpgaI-knl-1'
                    price = fpga_price(1); %1.65;
                otherwise 
                    error('No option selected');    
            end
            cost_1ker(id,iKM,ik) = time_1ker(id,iKM,ik)/ 3600 * price;
            % Store .txt for paper
            filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-cost.txt');
            fileID = fopen(filename,'w');
            fprintf(fileID,'%4.2f',cost_1ker(id,iKM,ik));
            ST = fclose(fileID);assert(ST==0)
            clearvars filename fileID ST
            %%%%%%%%%%%%%%%%%%%%%%%
            % energy_1ker
            switch device
                case 'cpu-cores1'
                    watts = power_cpu *cpu_cores(1); %8;  %(2/96)*384
                case 'fpgaI-knl-1'
                    watts = power_fpga_single_kernel*fpga_cores(1); %33;
                otherwise 
                error('No option selected');    
            end
            energy_1ker(id,iKM,ik) = time_1ker(id,iKM,ik) * watts;
            % Store .txt for paper
            filename = strcat(OUTPUTFOLDER,device,'-nKM',NKM,'-nk',Nk,'-energy.txt');
            fileID = fopen(filename,'w');
            fprintf(fileID,'%4.2f',energy_1ker(id,iKM,ik));
            ST = fclose(fileID);assert(ST==0)
            clearvars filename fileID ST
            clearvars chip device price watts

        end
    end
end

%% Step 4: Single-Kernel Design: Speedup, cost savings, energy savings across grid sizes (iKM,ik)
for iKM=1:length(KMlist)
    NKM = KMlist{iKM};
    for ik=1:length(klist)
        Nk = klist{ik};
        % Speedup
        speedup_1ker(iKM,ik) = (time_1ker(1,iKM,ik)/time_1ker(2,iKM,ik));
        filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-knl-1-nKM',NKM,'-nk',Nk,'-speedup','.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',speedup_1ker(iKM,ik));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST
        
        % Cost Savings
        costsavings_1ker(iKM,ik) = (cost_1ker(2,iKM,ik)/cost_1ker(1,iKM,ik))*100;
        filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-knl-1-nKM',NKM,'-nk',Nk,'-costsavings','.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',costsavings_1ker(iKM,ik));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST

        % Energy Savings
        energysavings_1ker(iKM,ik) = (energy_1ker(2,iKM,ik)/energy_1ker(1,iKM,ik))*100;
        filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-knl-1-nKM',NKM,'-nk',Nk,'-energysavings','.txt');
        fileID = fopen(filename,'w');
        fprintf(fileID,'%4.2f',energysavings_1ker(iKM,ik));
        ST = fclose(fileID);assert(ST==0)
        clearvars filename fileID ST

    end
end

%% Step 3. Create all tables
% Create tables in the paper
clearvars ans chip cpu_corelist coreslist  device devicelist fpga_corelist icpu id ifpga ik iKM inumberofchips 

%% Table 2: Benchmarking the CPU: Alternative Search Algorithms
% Linear Search
filename = strcat(INPUTFOLDER,'time/','cpu-cores1-linear-kernel-time.txt');
fileID = fopen(filename,'r');
cpu_time_linear_search = fscanf(fileID,'%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Store .txt for paper
filename = strcat(OUTPUTFOLDER,'cpu-cores1-linear-kernel-time.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%6.1f',cpu_time_linear_search);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Binary Search
filename = strcat(INPUTFOLDER,'time/','cpu-cores1-binary-kernel-time.txt');
fileID = fopen(filename,'r');
cpu_time_binary_search= fscanf(fileID,'%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Store .txt for paper
filename = strcat(OUTPUTFOLDER,'cpu-cores1-binary-kernel-time.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%6.1f',cpu_time_binary_search);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Jump Search
filename = strcat(INPUTFOLDER,'time/','cpu-cores1-nKM4-nk100-kernel-time.txt');
fileID = fopen(filename,'r');
cpu_time_jump_search= fscanf(fileID,'%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Store .txt for paper
filename = strcat(OUTPUTFOLDER,'cpu-cores1-nKM4-nk100-kernel-time.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%6.1f',cpu_time_jump_search);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Table
table_mat = zeros(2,3);
table_mat(1,:)=[cpu_time_linear_search,cpu_time_binary_search,cpu_time_jump_search];
table_mat(2,:)=[1,cpu_time_linear_search/cpu_time_binary_search,cpu_time_linear_search/cpu_time_jump_search];
% Table cell
table_cell=cell(3,4);
% Headings
table_cell(1,2:end) ={{'Linear Search'},{'Binary Search'},{'Jump Search'}};
table_cell(2,1) ={'Solution Time (s)'};
table_cell(3,1) ={'Speedup'};
table_cell(2:end, 2:end) = num2cell(table_mat);
tabella='table2';save([TABLEFOLDER,tabella,'/table.mat'], 'table_cell','table_mat');eval(strcat(tabella,'=table_cell;'));
clearvars table_cell table_mat tabella

% Store .txt for paper
filename = strcat(OUTPUTFOLDER,'cpu-cores1-binary-speedup.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%3.2f',cpu_time_linear_search/cpu_time_binary_search);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Store .txt for paper
filename = strcat(OUTPUTFOLDER,'cpu-cores1-jumpsearch-speedup.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%3.2f',cpu_time_linear_search/cpu_time_jump_search);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

%% Table A4: Time, Cost, Energy across devices
table_cell=cell(6,7);
% Column Headings
row = 1;
col = 2;
for i=1:length(devicelistcpu)
    table_cell(row,col)=devicelistcpu(i);col=col+1;
end
for i=1:length(devicelistfpga)
    table_cell(row,col)=devicelistfpga(i);col=col+1;
end
% Execution Time
row = row+1;
col = 1;
table_cell(row,col)= {'Time (s)'};col = col+1;
for i=1:length(devicelistcpu)
    table_cell(row,col)={round(exectime(1,i),2)};col=col+1;
end
for i=1:length(devicelistfpga)
    table_cell(row,col)={round(exectime(2,i),2)};col=col+1;
end
% Initialization Time
row = row+1;
col = 1;
table_cell(row,col)= {'Time (s)'};col = col+1;
for i=1:length(devicelistcpu)
    table_cell(row,col)={round(inittime(1,i),2)};col=col+1;
end
for i=1:length(devicelistfpga)
    table_cell(row,col)={round(inittime(2,i),2)};col=col+1;
end
% Printing Time
row = row+1;
col = 1;
table_cell(row,col)= {'Time (s)'};col = col+1;
for i=1:length(devicelistcpu)
    table_cell(row,col)={round(writetime(1,i),2)};col=col+1;
end
for i=1:length(devicelistfpga)
    table_cell(row,col)={round(writetime(2,i),2)};col=col+1;
end
% Solution Time
row = row+1;
col = 1;
table_cell(row,col)= {'Solution Time (s)'};col = col+1;
for i=1:length(devicelistcpu)
    table_cell(row,col)={round(time(1,i),2)};col=col+1;
end
for i=1:length(devicelistfpga)
    table_cell(row,col)={round(time(2,i),2)};col=col+1;
end
% Cost
row = row+1;
col = 1;
table_cell(row,col)= {'Cost ($)'};col = col+1;
for i=1:length(devicelistcpu)
    table_cell(row,col)={round(cost(1,i),2)};col=col+1;
end
for i=1:length(devicelistfpga)
    table_cell(row,col)={round(cost(2,i),2)};col=col+1;
end
% Energy
row = row+1;
col = 1;
table_cell(row,col)= {'Energy (J)'};col = col+1;
for i=1:length(devicelistcpu)
    table_cell(row,col)={round(energy(1,i),2)};col=col+1;
end
for i=1:length(devicelistfpga)
    table_cell(row,col)={round(energy(2,i),2)};col=col+1;
end
tabella='tableA4';save([TABLEFOLDER,tabella,'/table.mat'], 'table_cell');eval(strcat(tabella,'=table_cell;'));
clearvars table_cell tabella
%% Table 3: Efficiency Gains of FPGA Acceleration
s_ = speedup;
c_ = costsavings;
e_ = energysavings;
table_mat =round([s_(1,1),s_(1,2),s_(1,3),c_(1,1),c_(1,2),c_(1,3),e_(1,1),e_(1,2),e_(1,3); ...
    s_(2,1),s_(2,2),s_(2,3),c_(2,1),c_(2,2),c_(2,3),e_(2,1),e_(2,2),e_(2,3); ...
    s_(3,1),s_(3,2),s_(3,3),c_(3,1),c_(3,2),c_(3,3),e_(3,1),e_(3,2),e_(3,3);] ...
    ,2);
clearvars s_ c_ e_
% Create Table
table_cell  = cell(size(table_mat, 1) + 1, size(table_mat, 2) + 1);
col=2;
for i=0:2
    switch i
        case 0
            heading_cell = "speedup";
        case 1
            heading_cell = "cost";
        case 2
            heading_cell = "energy";
        otherwise 
            error('No option selected');    
    end
    table_cell(1,col)={strcat(heading_cell,'-',devicelistfpga(1))};col=col+1;
    table_cell(1,col)={strcat(heading_cell,'-',devicelistfpga(2))};col=col+1;
    table_cell(1,col)={strcat(heading_cell,'-',devicelistfpga(3))};col=col+1;
end
%table_cell(1, 2:end) = {devicelistfpga,devicelistfpga,devicelistfpga};
table_cell(2:end, 1) = devicelistcpu;
table_cell(2:end, 2:end) = num2cell(table_mat);
tabella='table3';save([TABLEFOLDER,tabella,'/table.mat'], 'table_cell', 'table_mat');eval(strcat(tabella,'=table_cell;'));
clearvars table_mat table_cell tabella

%% Table 4 - Panel A: Single-kernel FPGA vs. Single CPU Core
table_cell=cell(2,5);
table_cell(1,:)={'FPGA-Time(sec)','CPU-Time(sec)','Speedup(x)','Relative Costs(%)','Energy(%)'};
col=1;
% -------------------------------------------------
% 'FPGA-Time(sec)'
filename = strcat(INPUTFOLDER,'time/','fpgaI-knl-1-nKM4-nk100-kernel-time.txt');
fileID = fopen(filename,'r');
fpgatime4 = fscanf(fileID,'%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Store .txt for paper
filename = strcat(OUTPUTFOLDER,'fpgaI-knl-1-nKM4-nk100-kernel-time.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',fpgatime4);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
fpga_avg4 = fpgatime4/tot_economies;
filename = strcat(OUTPUTFOLDER,'fpgaI-knl-1-nKM4-nk100-time-avg.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',fpga_avg4);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Table
table_cell(2,col)={round(fpga_avg4,2)};col=col+1;
% -------------------------------------------------
% 'CPU-Time(sec)'
cputime4 = time(1,1);
cpu_avg4 = cputime4/tot_economies;
filename = strcat(OUTPUTFOLDER,'cpu-cores1-nKM4-nk100-time-avg.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',cpu_avg4);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'cpu-cores1-nKM4-nk100-time-avg_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(cpu_avg4));
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Table
table_cell(2,col)={round(cpu_avg4,2)};col=col+1;
% -------------------------------------------------
% 'Speedup(x)'
speedup4 = (cputime4/fpgatime4);
filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-knl-1-nKM4-nk100-speedup.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',speedup4);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Table
table_cell(2,col)={round(speedup4,2)};col=col+1;
% -------------------------------------------------
% Relative Costs(%)
cost4 = fpgatime4/3600 * fpga_price(1);
filename = strcat(OUTPUTFOLDER,'fpgaI-datapar-knl-1-nKM4-nk100-cost.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',cost4);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
costsavings4 = cost4/cost(1,1)*100;
filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-knl-1-nKM4-nk100-costsavings.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',costsavings4);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Table
table_cell(2,col)={round(costsavings4,2)};col=col+1;
% -------------------------------------------------
% Energy(%)
energy4 = fpgatime4*power_fpga_single_kernel;
filename = strcat(OUTPUTFOLDER, 'fpgaI-datapar-knl-1-nKM4-nk100-energy.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',energy4);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
energysavings4 = 100*energy4/energy(1,1);
filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-knl-1-nKM4-nk100-energysavings.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',energysavings4);
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Table
table_cell(2,col)={round(energysavings4,2)};col=col+1;
tabella='table4_panel_A';save([TABLEFOLDER,'table4/panel_A.mat'], 'table_cell');eval(strcat(tabella,'=table_cell;'));
clearvars table_cell tabella cputime4 fpgatime4 cost4 costsavings4 energy4 energysavings4 table_cell fpga_avg4 cpu_avg4

%% Table 4 - Panel B + A5: Time performances across different grid sizes from time_1ker(device,NKM,Nk)
table_speedup = horzcat(speedup_1ker(1,:),speedup_1ker(2,:)); % Speedup over 4-100,4-200,4-300 AND 8-100,8-200,8-300
table_costsavings = horzcat(costsavings_1ker(1,:),costsavings_1ker(2,:)); % Cost savings over 4-100,4-200,4-300 AND 8-100,8-200,8-300
table_energysavings= horzcat(energysavings_1ker(1,:),energysavings_1ker(2,:));

% Table A5
table_mat = [squeeze(exectime_1ker(1,1,:))',squeeze(exectime_1ker(1,2,:))';...
                      squeeze(inittime_1ker(1,1,:))', squeeze(inittime_1ker(1,2,:))';...
                   squeeze(writetime_1ker(1,1,:))',squeeze(writetime_1ker(1,2,:))';...
                          squeeze(time_1ker(1,1,:))',squeeze(time_1ker(1,2,:))';...
                          squeeze(cost_1ker(1,1,:))',squeeze(cost_1ker(1,2,:))';...
                          squeeze(energy_1ker(1,1,:))',squeeze(energy_1ker(1,2,:))']; 
table_cell=cell(7,7);
% Headings Columns (NKM,nk)
table_cell(1,2:end)={{'(4,100)'},{'(4,200)'},{'(4,300)'},{'(8,100)'},{'(8,200)'},{'(8,300)'}};
% Headings Rows
table_cell(2,1)={'Exec. Time (s)'};
table_cell(3,1)={'Init. Time (s)'};
table_cell(4,1)={'Print. Time (s)'};
table_cell(5,1)={'Sol. Time (s)'};
table_cell(6,1)={'Cost ($)'};
table_cell(7,1)={'Energy (J)'};
table_cell(2:end, 2:end) = num2cell(table_mat);
tabella='tableA5';save([TABLEFOLDER,tabella,'/table.mat'], 'table_cell','table_mat');eval(strcat(tabella,'=table_cell;'));
clearvars table_cell table_mat tabella

% Table 4- Panel B: Speedup
table_mat = [table_speedup;table_costsavings;table_energysavings];
clearvars table_speedup table_costsavings table_energysavings
table_cell=cell(4,7);
table_cell(1,2:end)={{'(4,100)'},{'(4,200)'},{'(4,300)'},{'(8,100)'},{'(8,200)'},{'(8,300)'}};
table_cell(2,1)={'Speedup (x)'};
table_cell(3,1)={'Relative Costs (%)'};
table_cell(4,1)={'Energy (%)'};
table_cell(2:end, 2:end) = num2cell(table_mat);
tabella='table4_panel_B';save([TABLEFOLDER,'table4/panel_B.mat'], 'table_cell','table_mat');eval(strcat(tabella,'=table_cell;'));
clearvars table_cell table_mat tabella

%% Table 5: Speedup Gains: Acceleration Channels Accounting
table_cell=cell(2,4);
table_cell(1,:)={'Baseline','Pipelining','Within-Economy','Across-Economies'};col=1;

% Execution time of 1-cpu core for 1200 economies
filename = strcat(INPUTFOLDER,'time/','cpu-cores1-nKM4-nk100-kernel-time.txt'); 
fileID = fopen(filename,'r');
cputime5 = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

designlist = {'fpgaI-base-knl-1','fpgaI-pip-knl-1','fpgaI-knl-1','fpgaI'};
for idesign=1:length(designlist)
    % Time
    filename = strcat(INPUTFOLDER,'time/',designlist{idesign},'-nKM4-nk100-kernel-time.txt');
    fileID = fopen(filename,'r');
    fpgatime5(idesign) = fscanf(fileID, '%f');
    ST = fclose(fileID);assert(ST==0)
    clearvars filename fileID ST

    tot_economies_fpga = tot_economies;
    if(idesign<=2)
        tot_economies_fpga = tot_economies_120; % baseline and pipeline are computed over 120 economies.
    end
    %Speedup
    speedup8(idesign) = (cputime5/tot_economies)/(fpgatime5(idesign)/tot_economies_fpga);
    filename = strcat(OUTPUTFOLDER,'cpuI-base-knl-1','-',designlist{idesign},'-nKM4-nk100-','speedup.txt');
    fileID = fopen(filename,'w');
    fprintf(fileID,'%4.2f',speedup8(idesign));
    ST = fclose(fileID);assert(ST==0)
    clearvars filename fileID ST tot_economies_fpga
    table_cell(2,col)={round(speedup8(idesign),2)};col=col+1;
end
tabella='table5';save([TABLEFOLDER,tabella,'/table.mat'], 'table_cell');eval(strcat(tabella,'=table_cell;'));

clearvars table_cell tabella cputime5 fpgatime5 designlist speedup8


%% Accuracy: Tables 6 - Panel A: Coefficients
% Coefficients CPU: Floating point precision
filename = strcat(INPUTFOLDER,'coefficients/','coeffs_cpu_cores1_i0_of_1200_nKM4_nk100.txt');
fileID = fopen(filename,'r');
coeffs_float = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Coefficients FPGA: Fixed-point precision
filename = strcat(INPUTFOLDER,'coefficients/','coeffs_fpgaI_nkM4_nk100_i0_d0_k0_of_1200.txt');
fileID = fopen(filename,'r');
coeffs_fixed = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
table_mat = [coeffs_float';coeffs_fixed'];

% Table
table_cell=cell(3,5);
% Headings
table_cell(1,2:end) = {{'beta1(b)'},{'beta2(b)'},{'beta1(g)'},{'beta2(g)'}};
table_cell(2,1)={'Floating-point'};
table_cell(3,1)={'Fixed-point'};
for col=1:4
    table_cell(2,1+col)={round(coeffs_float(col),4)};
    table_cell(3,1+col)={round(coeffs_fixed(col),4)};
end
tabella='tableA6_panel_A';save([TABLEFOLDER,'tableA6/panel_A.mat'], 'table_cell','table_mat');eval(strcat(tabella,'=table_cell;'));
clearvars table_cell table_mat tabella

% Assert that moment of the distributions are identical up to the 9th decimal place
diff = max(abs(coeffs_float-coeffs_fixed)',[],2);
assert(diff<=1e-9)
clearvars diff

% Store floating point coefficients .txt
filename = strcat(OUTPUTFOLDER,'tab_betaIab.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%5.4f',coeffs_float(1)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_betaIIab.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%5.4f',coeffs_float(2)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_betaIag.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%5.4f',coeffs_float(3)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_betaIIag.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%5.4f',coeffs_float(4)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Store fixed point coefficients .txt
filename = strcat(OUTPUTFOLDER,'tab_betaIabFix.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%5.4f',coeffs_fixed(1)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_betaIIabFix.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%5.4f',coeffs_fixed(2)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_betaIagFix.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%5.4f',coeffs_fixed(3)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_betaIIagFix.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%5.4f',coeffs_fixed(4)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
clearvars coeffs_float coeffs_fixed

%% Accuracy: Tables 6 - Panel B: Policy Functions
% Policy Functions k', CPU
filename = strcat(INPUTFOLDER,'kprime/','kpo_cpu_cores1_i0_of_1200_nKM4_nk100.txt');
fileID = fopen(filename,'r');
kprime_float = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Policy Functions k', FPGA
filename = strcat(INPUTFOLDER,'kprime/','kpo_fpgaI_nkM4_nk100_i0_d0_k0_of_1200.txt');
fileID = fopen(filename,'r');
kprime_fixed = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Statistics
rel_diff = abs(kprime_fixed - kprime_float)./kprime_float*100;      % 1600x1
mean_rel_diff = mean(rel_diff,'omitnan');
max_rel_diff = max(rel_diff);
% Table
table_cell=cell(1,4);
table_cell(1,1)={'Mean(|Fixed−Float|/Float) %'};table_cell(1,2)={mean_rel_diff};
table_cell(1,3)={'Max(|Fixed−Float|/Float) %'};table_cell(1,4)={max_rel_diff};
tabella='tableA6_panel_B';save([TABLEFOLDER,'tableA6/panel_B.mat'], 'table_cell');eval(strcat(tabella,'=table_cell;'));
clearvars table_cell rel_diff kprime_float kprime_fixed

% Store .txt
filename = strcat(OUTPUTFOLDER,'tab_mean_rel_diff_kprime.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%2.1e',mean_rel_diff); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_max_rel_diff_kprime.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%2.1e',max_rel_diff); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
clearvars mean_rel_diff max_rel_diff

%% Accuracy: Tables 6 - Panel C: Distributions
% Individual capital holding distribution in the last period, CPU
filename = strcat(INPUTFOLDER,'kcross/','kcross_cpu_cores1_i0_of_1200_nKM4_nk100.txt');
fileID = fopen(filename,'r');
kcross_float = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Individual capital holding distribution  in the last period, FPGA
filename = strcat(INPUTFOLDER,'kcross/','kcross_fpgaI_nkM4_nk100_i0_d0_k0_of_1200.txt');
fileID = fopen(filename,'r');
kcross_fixed = fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

float_mean = round(mean(kcross_float),2);
float_std    = round(std(kcross_float),2);
float_Q1    = round(quantile(kcross_float, 0.25),2);
float_Q2    = round(quantile(kcross_float, 0.50),2);
float_Q3    = round(quantile(kcross_float, 0.75),2);

fixed_mean = round(mean(kcross_fixed),2);
fixed_std    = round(std(kcross_fixed),2);
fixed_Q1    = round(quantile(kcross_fixed, 0.25),2);
fixed_Q2    = round(quantile(kcross_fixed, 0.50),2);
fixed_Q3    = round(quantile(kcross_fixed, 0.75),2);

% Assert that moment of the distributions are identical up to the 7th decimal place
diff = max([abs(mean(kcross_float)-mean(kcross_fixed)), ...
                  abs(std(kcross_float)-std(kcross_fixed)),...
                  abs(quantile(kcross_float, 0.25)-quantile(kcross_fixed, 0.25)),...
                  abs(quantile(kcross_float, 0.50)-quantile(kcross_fixed, 0.50)),...
                  abs(quantile(kcross_float, 0.75)-quantile(kcross_fixed, 0.75))]);
assert(diff<=1e-7)
clearvars diff

rel_diff = abs(kcross_fixed - kcross_float)./kcross_float*100;      % 1600x1

% Table
table_cell=cell(4,6);
% Headings
table_cell(1,2:end) = {{'Mean'},{'Std'},{'Q1'},{'Q2'},{'Q3'}};
table_cell(2,1)={'Floating-point'};table_cell(2,2)={float_mean};table_cell(2,3)={float_std};table_cell(2,4)={float_Q1};table_cell(2,5)={float_Q2};table_cell(2,6)={float_Q3};
table_cell(3,1)={'Fixed-point'};   table_cell(3,2)={fixed_mean};table_cell(3,3)={fixed_std};table_cell(3,4)={fixed_Q1};table_cell(3,5)={fixed_Q2};table_cell(3,6)={fixed_Q3};
% Mean and Max relative differences
table_cell(4,1)={'Mean(|Fixed−Float|/Float) %'};table_cell(4,2)={mean(rel_diff)};
table_cell(4,3)={'Max(|Fixed−Float|/Float) %'};table_cell(4,4)={max(rel_diff)};
tabella='tableA6_panel_C';save([TABLEFOLDER,'tableA6/panel_C.mat'], 'table_cell');eval(strcat(tabella,'=table_cell;'));
clearvars table_cell kcross_float kcross_fixed

% Store floating point statistics .txt
filename = strcat(OUTPUTFOLDER,'tab_meankcross.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',float_mean); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_stdkcross.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%5.2f',float_std); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_qIkcross.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',float_Q1); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_mediankcross.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',float_Q2); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_qIIIkcross.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',float_Q3); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Store fixed point statistics .txt
filename = strcat(OUTPUTFOLDER,'tab_meankcrossFix.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',fixed_mean); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_stdkcrossFix.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%5.2f',fixed_std); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_qIkcrossFix.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',fixed_Q1); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_mediankcrossFix.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',fixed_Q2); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_qIIIkcrossFix.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',fixed_Q3); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Store .txt
filename = strcat(OUTPUTFOLDER,'tab_mean_rel_diff_kcross.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%2.1e',mean(rel_diff)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'tab_max_rel_diff_kcross.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%2.1e',max(rel_diff)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
clearvars mean_rel_diff max_rel_diff

clearvars float_mean float_std float_Q1 float_Q2 float_Q3
clearvars fixed_mean fixed_std fixed_Q1 fixed_Q2 fixed_Q3
clearvars rel_diff

%% Accuracy: Tables 6 - Panel D: Euler Equation Errors

%%%%%%%%%%%%%%%%%%%%%%%%%
% Euler Equation Errors
table_cell=cell(5,5);
table_cell(1,:)={'Nk','EEE','FPGA','CPU','Delta'};col=1;
nklist = [100,300];
NKM = 4;
row = 2;
for i=1:length(nklist)
    nk = nklist(i);
    [Table_EE_FPGA,Table_EE_CPU,Table_relative_EE] = EEE_fun(nk,NKM,INPUTFOLDER,[TABLEFOLDER,'tableA6/EEErrors/']);
    table_cell(row,1)={nk};
    table_cell(row,2)=Table_EE_FPGA(1,1); 
    table_cell(row,3)=Table_EE_FPGA(1,2); 
    table_cell(row,4)=Table_EE_CPU(1,2);   
    table_cell(row,5)=Table_relative_EE(1,2);
    row=row+1;
    table_cell(row,1)={nk};
    table_cell(row,2)=Table_EE_FPGA(2,1);
    table_cell(row,3)=Table_EE_FPGA(2,2);
    table_cell(row,4)=Table_EE_CPU(2,2);
    table_cell(row,5)=Table_relative_EE(2,2);
    row=row+1;

    % Store Mean EEE
    filename = strcat(OUTPUTFOLDER,'tab_EEE_FPGA_nKM',num2str(NKM),'_nk',num2str(nk),'_mean.txt');
    fileID = fopen(filename,'w');
    fprintf(fileID,'%3.2f',Table_EE_FPGA{1,2}); 
    ST = fclose(fileID);assert(ST==0)
    clearvars filename fileID ST
    filename = strcat(OUTPUTFOLDER,'tab_EEE_CPU_nKM',num2str(NKM),'_nk',num2str(nk),'_mean.txt');
    fileID = fopen(filename,'w');
    fprintf(fileID,'%3.2f',Table_EE_CPU{1,2}); 
    ST = fclose(fileID);assert(ST==0)
    clearvars filename fileID ST
    filename = strcat(OUTPUTFOLDER,'tab_EEE_relative_nKM',num2str(NKM),'_nk',num2str(nk),'_mean.txt');
    fileID = fopen(filename,'w');
    fprintf(fileID,'%.2e',Table_relative_EE{1,2}); 
    ST = fclose(fileID);assert(ST==0)
    clearvars filename fileID ST
    % Store Max EEE
    filename = strcat(OUTPUTFOLDER,'tab_EEE_FPGA_nKM',num2str(NKM),'_nk',num2str(nk),'_max.txt');
    fileID = fopen(filename,'w');
    fprintf(fileID,'%3.2f',Table_EE_FPGA{2,2}); 
    ST = fclose(fileID);assert(ST==0)
    clearvars filename fileID ST
    filename = strcat(OUTPUTFOLDER,'tab_EEE_CPU_nKM',num2str(NKM),'_nk',num2str(nk),'_max.txt');
    fileID = fopen(filename,'w');
    fprintf(fileID,'%3.2f',Table_EE_CPU{2,2}); 
    ST = fclose(fileID);assert(ST==0)
    clearvars filename fileID ST
    filename = strcat(OUTPUTFOLDER,'tab_EEE_relative_nKM',num2str(NKM),'_nk',num2str(nk),'_max.txt');
    fileID = fopen(filename,'w');
    fprintf(fileID,'%.2e',Table_relative_EE{2,2}); 
    ST = fclose(fileID);assert(ST==0)
    clearvars filename fileID ST

    clearvars Table_EE_FPGA Table_EE_CPU Table_relative_EE
end
clearvars nklist NKM row
tabella='tableA6_panel_D';save([TABLEFOLDER,'tableA6/panel_D.mat'], 'table_cell');eval(strcat(tabella,'=table_cell;'));

% Verify that all differences are lower than 1e-6
diff = max([table_cell{2,5},table_cell{3,5},table_cell{4,5},table_cell{5,5}],[],2);
assert(diff<=1e-6)
clearvars table_cell diff

%% Save all Tables
save([TABLEFOLDER,'Tables.mat'], 'table2', 'table3', 'table4_panel_A','table4_panel_B', 'tableA6_panel_A', 'tableA6_panel_B', 'tableA6_panel_C', 'tableA6_panel_D', 'tableA4', 'tableA5');

%% Abstract
display("Abstract")
cpu_seconds = seconds(time(1,1));
cpu_seconds.Format = 'hh:mm';
cpu_hours = hours(cpu_seconds);
fpga_seconds = seconds(time(2,1));
fpga_seconds.Format = 'mm:ss';
fpga_minutes= minutes(fpga_seconds);
fprintf("Single-core vs FPGA-I Speedup %dx: from %s to %s \n",round(speedup(1,1)),cpu_seconds,fpga_seconds);
% Acceleration benchmark model
filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-nKM4-nk100-speedup_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(speedup(1,1)));
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Single Core: Solution benchmark model in hours
filename = strcat(OUTPUTFOLDER,'cpu-cores1-nKM4-nk100-hours_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(cpu_hours));
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% FPGA: Solution benchmark model in minutes
filename = strcat(OUTPUTFOLDER,'fpgaI-nKM4-nk100-minutes_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',ceil(fpga_minutes));
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

%% Introduction
display("Introduction")
% (100,4): Speedup CPU1-FPGAIII
filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaIII-nKM4-nk100-speedup_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(speedup(1,3))); % (cpu-devices,fpga-devices) 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% (100,4): Speedup CPU8-FPGAIII
filename = strcat(OUTPUTFOLDER,'cpu-cores8-fpgaIII-nKM4-nk100-speedup_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(speedup(2,3))); % (cpu-devices,fpga-devices) 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% (100,4): Speedup CPU48-FPGAIII
filename = strcat(OUTPUTFOLDER,'cpu-cores48-fpgaIII-nKM4-nk100-speedup_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(speedup(3,3))); % (cpu-devices,fpga-devices) 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
% Max cost savings
filename = strcat(OUTPUTFOLDER,'maxcostsavings_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(max(costsavings,[],'all'))); % (cpu-devices,fpga-devices) 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
filename = strcat(OUTPUTFOLDER,'maxenergysavings_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(max(energysavings,[],'all'))); % (cpu-devices,fpga-devices) 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

%% Quantitative Results
display("Quantitative Results")
%%%%%%%%%%%%%%%%%%
% SPEEDUPS
% 8-Core: Solution benchmark model in hours
cpuII_seconds = seconds(time(1,2));
cpuII_seconds.Format = 'hh:mm';
cpuII_hours = hours(cpuII_seconds);
filename = strcat(OUTPUTFOLDER,'cpu-cores8-nKM4-nk100-hours_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(cpuII_hours));
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

%%%%%%%%%%%%%%%%%%
% COST SAVINGS
% CPU: cost one million economies
filename = strcat(OUTPUTFOLDER,'cpu-cores1-nKM4-nk100-cost-million_economies.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(cost(1,1)/tot_economies*one_million_economies)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% FPGA: cost one million economies
filename = strcat(OUTPUTFOLDER,'fpgaI-nKM4-nk100-cost-million_economies.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(cost(2,1)/tot_economies*one_million_economies)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

%% Robustness
% Theoretical speedup
filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-nKM4-nk100-speedup-theoretical.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(speedup_1ker(1,1)*3)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Power Consumption Single Kernel design
filename = strcat(OUTPUTFOLDER,'carbfoot_fpgaI-knl-1-nKM4-nk100-power.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',power_fpga_single_kernel); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

%% Inspecting the Mechanism

% Baseline
filename = strcat(INPUTFOLDER,'time/fpgaI-base-knl-1-nKM4-nk100-kernel-time.txt');
fileID = fopen(filename,'r');
fpgatime_baseline= fscanf(fileID, '%f');
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

fpgatime_baseline_average = fpgatime_baseline/tot_economies_120;
cpu_time_avg = time(1,1)/tot_economies;

% FPGA baseline average time
filename = strcat(OUTPUTFOLDER,'fpgaI-base-knl-1-nKM4-nk100-time-avg_rounded.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',round(fpgatime_baseline_average)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% CPU 1 vs FPGA baseline speedup
fpga_base_cpu_speedup = round(fpgatime_baseline_average/cpu_time_avg);
if(fpga_base_cpu_speedup==5)
    fpga_base_cpu_speedup_word = 'five';
else
    disp('ERROR')
    error(-1);
end
filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-knl-1-base-nKM4-nk100-speedup-word.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%s',fpga_base_cpu_speedup_word); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Precision-accuracy analysis
filename = strcat(OUTPUTFOLDER,'tab_kcrosskprimemaxdiff.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%.1e',max([tableA6_panel_B{1,2}, tableA6_panel_B{1,4}, tableA6_panel_C{4,2}, tableA6_panel_C{4,4}],[],2)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Within-data parallelism
speedup_ihp=(Performance_.cpu_ihp_sh*time(1,1)/tot_economies)/(Performance_.fpga_ihp*1e-3);
speedup_sim=(Performance_.cpu_sim_sh*time(1,1)/tot_economies)/(Performance_.fpga_sim*1e-3);

% Benchmark model: Speedup single-kernel vs  CPU: Simulation
filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-knl-1-nKM4-nk100-speedup-sim.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%.0f',round(speedup_sim)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Benchmark model: Speedup single-kernel vs  CPU: IHP
filename = strcat(OUTPUTFOLDER,'cpu-cores1-fpgaI-knl-1-nKM4-nk100-speedup-ihp.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%.0f',round(speedup_ihp)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Benchmark model: Execution time (in milliseconds) of Simulation Step
filename = strcat(OUTPUTFOLDER,'fpgaI-knl-1-nKM4-nk100-sim-time-tot.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%.0f',round(Performance_.fpga_sim)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

% Benchmark model: Execution time (in milliseconds) of IHP
filename = strcat(OUTPUTFOLDER,'fpgaI-knl-1-nKM4-nk100-ihp-time-tot.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%.0f',round(Performance_.fpga_ihp)); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

clearvars speedup_ihp speedup_sim
clearvars fpgatime_baseline fpgatime_baseline_average cpu_time_avg fpga_base_cpu_speedup fpga_base_cpu_speedup_word

%% Carbon Footprint
display('Average Pound of CO2 for Xcel Colorado kWh')
lbs_CO2_kWh = w_gas * CO2_per_kWh_gas + w_coal * CO2_per_kWh_coal + w_ren * CO2_per_kWh_ren
lbs_per_metric_ton = 2204.62;
joules_kWh = 1/3600*1/1000; % 1J = joules_kWh * kWh, % Joules (watts/second) to kWh (watts * hour / 1000)

%%% Summit on CPU
watts = summit_power;       % in Watts
tot_core_hour_per_year = Tot_core_hour_summit;
% ---
display('Carbon Footprint Summit - CPU')
kWh_core_hour=watts/1000                                                            % kWh per core hour
lbs_CO2_core_hour = lbs_CO2_kWh * kWh_core_hour                       % lbs CO2 per coure hour
lbs_CO2_per_year = lbs_CO2_core_hour * tot_core_hour_per_year    % lbs CO2 per year
metric_tons_CO2 = lbs_CO2_per_year / lbs_per_metric_ton               % metric tons of CO2 per year
cars_worth_of_CO2 = round(metric_tons_CO2/metric_tons_CO2_per_car)                          % metric tons of CO2 per year in cars
% ---
%summit_cpu = [metric_tons_CO2,metric_tons_CO2_cars];

%%% Summit on FPGA
watts = power_fpga;
% ---
display('Carbon Footprint Summit - FPGA')
tot_core_hour_per_year = Tot_core_hour_summit/speedup(1,1)
kWh_core_hour=watts/1000                                                                % kWh per core hour
lbs_CO2_core_hour = lbs_CO2_kWh * kWh_core_hour                           % lbs CO2 per coure hour
lbs_CO2_per_year = lbs_CO2_core_hour * tot_core_hour_per_year         % lbs CO2 per year
metric_tons_CO2 = lbs_CO2_per_year / lbs_per_metric_ton                    % metric tons of CO2 per year
cars_worth_of_CO2 = round(metric_tons_CO2/metric_tons_CO2_per_car)  % metric tons of CO2 per year in cars

% Store .txt
filename = strcat(OUTPUTFOLDER,'carbfoot_fpgaI-nKM4-nk100-power.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',power_fpga); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

filename = strcat(OUTPUTFOLDER,'carbfoot_fpgaI-nKM4-nk100-power_kWh.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.3f',power_fpga/1000); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

filename = strcat(OUTPUTFOLDER,'carbfoot_lbsC02_fpgahour.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.3f',lbs_CO2_core_hour); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

filename = strcat(OUTPUTFOLDER,'carbfoot_Summit_yearlyhour_on_fpga.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%s',regexprep(num2str(round(tot_core_hour_per_year)),'(?<!\.\d*)\d{1,3}(?=(\d{3})+\>)','$&,')); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

filename = strcat(OUTPUTFOLDER,'carbfoot_lbsC02_year.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%s',regexprep(num2str(round(lbs_CO2_per_year)),'(?<!\.\d*)\d{1,3}(?=(\d{3})+\>)','$&,')); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

filename = strcat(OUTPUTFOLDER,'carbfoot_metrictonsC02_year.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%4.2f',metric_tons_CO2); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

filename = strcat(OUTPUTFOLDER,'carbfoot_cars_fpga.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%d',cars_worth_of_CO2); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST

if(cars_worth_of_CO2==5)
    cars_worth_of_CO2_word= 'five'
elseif(cars_worth_of_CO2==6) 
     cars_worth_of_CO2_word = 'six'
else
    display('Different number of cars than five or six')
    error('USER-ERROR: Number to String for C02 in car consumption absent. Add it.');
end

filename = strcat(OUTPUTFOLDER,'carbfoot_cars_fpga_word.txt');
fileID = fopen(filename,'w');
fprintf(fileID,'%s',cars_worth_of_CO2_word); 
ST = fclose(fileID);assert(ST==0)
clearvars filename fileID ST
