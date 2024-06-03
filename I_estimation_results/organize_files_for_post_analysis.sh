#!/bin/bash

BASE=$(pwd)

RESULTS_DIR="s3-bucket/results"
CPU_RESULTS="$BASE/$RESULTS_DIR/cpu"
FPGA_RESULTS="$BASE/$RESULTS_DIR/fpga"

COEFF_DIR="$BASE/coefficients"
KCROSS_DIR="$BASE/kcross"
KPRIME_DIR="$BASE/kprime"
TIME_DIR="$BASE/time"
POWER_DIR="$BASE/power"
RESOURCES_DIR="$BASE/resources"
REPORTS_DIR="$BASE/reports"

echo -e "Step 1. Cleaning the folders:\n $COEFF_DIR\n $KCROSS_DIR\n $KPRIME_DIR\n $TIME_DIR\n"
rm $COEFF_DIR/coeffs*.txt
rm $KCROSS_DIR/kcross*.txt
rm $KPRIME_DIR/kpo*.txt
rm $TIME_DIR/*time-tot.txt
rm $TIME_DIR/*init-time.txt
rm $TIME_DIR/*write-time.txt
rm $TIME_DIR/*kernel-time.txt
rm $POWER_DIR/power_*.txt
rm $RESOURCES_DIR/xrt*.run_summary
rm $REPORTS_DIR/*.rpt
rm $REPORTS_DIR/log*.txt

echo "Post-analysis: Step 1. Organize the results in the respective folders"

#copy coefficients from results folder
cp -r "$CPU_RESULTS/"coeffs*.txt "$COEFF_DIR/" 
cp -r "$FPGA_RESULTS/"coeffs*.txt "$COEFF_DIR/" 

#copy kcross from results folder 
cp -r "$CPU_RESULTS/"kcross*.txt "$KCROSS_DIR/" 
cp -r "$FPGA_RESULTS/"kcross*.txt "$KCROSS_DIR/" 

#copy kprime from results folder 
cp -r "$CPU_RESULTS/"kpo*.txt "$KPRIME_DIR/" 
cp -r "$FPGA_RESULTS/"kpo*.txt "$KPRIME_DIR/" 

# CPU: Copy execution time results
cp -r "$CPU_RESULTS/"*time-tot.txt "$TIME_DIR/"
cp -r "$CPU_RESULTS/"*init-time.txt "$TIME_DIR/" 
cp -r "$CPU_RESULTS/"*kernel-time.txt "$TIME_DIR/" 
cp -r "$CPU_RESULTS/"*write-time.txt "$TIME_DIR/" 
# FPGA: Copy execution time results
cp -r "$FPGA_RESULTS/"*time-tot.txt "$TIME_DIR/"
cp -r "$FPGA_RESULTS/"*init-time.txt "$TIME_DIR/"
cp -r "$FPGA_RESULTS/"*kernel-time.txt "$TIME_DIR/" 
cp -r "$FPGA_RESULTS/"*write-time.txt "$TIME_DIR/"

# Power Analysis
cp $FPGA_RESULTS/power_fpgaI_knl-1_nKM4_nk100_i0_d0_k0_of_1200.txt "$POWER_DIR/"  
cp $FPGA_RESULTS/power_fpgaI_nKM4_nk100_i0_d0_k0_of_1200.txt "$POWER_DIR/"  

# Resource Analysis
cp $FPGA_RESULTS/xrt*.run_summary "$RESOURCES_DIR/"  

# Reports Analysis
cp $FPGA_RESULTS/reports/1ker_100k_4km_runOnfpga_csynth.rpt "$REPORTS_DIR/"  
cp $FPGA_RESULTS/log_fpgaI_knl-1_nKM4_nk100_1200.txt "$REPORTS_DIR/"  
cp $FPGA_RESULTS/reports/baseline_1ker_100k_4km_runOnfpga_csynth.rpt "$REPORTS_DIR/"  

echo "Transfer of data completed."
