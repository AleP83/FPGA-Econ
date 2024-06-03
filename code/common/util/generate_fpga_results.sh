#!/bin/bash
BASE=$(pwd)
# Automatically collect number of cores in the f1 instance
CORES_PER_SOCKET=$(lscpu | grep 'Core(s) per socket:' | awk '{print $4}')
SOCKETS=$(lscpu | grep 'Socket(s):' | awk '{print $2}')
# Calculate total number of CPU cores
CPU_CORES=$((CORES_PER_SOCKET * SOCKETS))
echo "Number of CPU CORES: $CPU_CORES"
# Use cores to determine instance
if [ "$CPU_CORES" = "4" ]; then
    INSTANCE="f1.2xlarge"
elif [ "$CPU_CORES" = "8" ]; then
    INSTANCE="f1.4xlarge"
elif [ "$CPU_CORES" = "32" ]; then
    INSTANCE="f1.16xlarge"
else
    echo "This is not an f1 instance (as of March 2024)."
fi

# Number of Economies to run in Tables 3,4,5
NUM_ECON_TABLE_3=1200 # default value = 1200
NUM_ECON_TABLE_4=1200 # default value = 1200
NUM_ECON_TABLE_5=120 # default value = 120

# AWS S3 bucket to store the final timing results
S3_EXE_BUCKET_NAME="fpga-econ-ks"
S3_FPGA_DIR="executables/fpga"
S3_HOST_EXECUTABLES_DIR="executables/fpga/host_executables"
S3_FPGA_AFI_DIR="executables/fpga/fpga_afi"
S3_RESULTS_DIR="results/fpga"
AWS_REGION="us-west-2"

# Result directories on the f1 instance
RESULTS_DIR="results/fpga"
FINAL_VALUES_DIR="final_values"
LOG_RESULTS_DIR="log_results"

# Executables directories on the f1 instance
HOST_EXECUTABLES_DIR="./executables/fpga/host_executables"
FPGA_AFI_DIR="./executables/fpga/fpga_afi"

# Function to create directories if they don't exist
create_directories() {
    if [ ! -d "$1" ]; then
        mkdir -p "$1"
    fi
}

# Check if the AWS CLI is configured
if ! aws configure get aws_access_key_id >/dev/null 2>&1; then \
    echo "Error: AWS CLI is not configured. Please run 'aws configure' to set up your AWS credentials before running this target."; \
    exit 1; \
fi

# If the argument is passed to use executables from S3 bucket, copy the executables to local directory
if [ "$USE_AWS_S3_EXE" == "yes" ]; then
    # Check if the bucket exists
    if ! aws s3 ls "s3://$S3_EXE_BUCKET_NAME" --region "$AWS_REGION"; then
        echo "Error: S3 bucket does not exist."
        exit 1
    else
        echo "S3 bucket '$S3_EXE_BUCKET_NAME' exists."
    fi

    # Copy executables from AWS S3 bucket to local folder
    create_directories "$HOST_EXECUTABLES_DIR"
    create_directories "$FPGA_AFI_DIR"
    aws s3 cp --recursive "s3://$S3_EXE_BUCKET_NAME/$S3_HOST_EXECUTABLES_DIR/" $HOST_EXECUTABLES_DIR/
    aws s3 cp --recursive "s3://$S3_EXE_BUCKET_NAME/$S3_FPGA_AFI_DIR/" $FPGA_AFI_DIR/
    if [ $? -ne 0 ]; then
        echo "Error: Failed to copy executables from S3 bucket."
        exit 1
    else
        echo "Executables copied successfully from S3 bucket."
    fi
else
    echo "Using the pre-compiled executables from the repository"
fi

# Function to run a test case
run_test_case() {
    local test_name=$1                  # output file txt name
    local host_executable=$2            # host executable name
    local fpga_afi=$3                   # fpga afi name

    # Create directories using the name provided as input
    create_directories "$RESULTS_DIR/$FINAL_VALUES_DIR"
    create_directories "$RESULTS_DIR/$LOG_RESULTS_DIR"

    # Aggregate grid size (e.g. 4)
    NKMGRID=$(echo "$host_executable" | sed -n 's/.*k_\([0-9]*\)km$/\1/p')
    # Individual capital grid size (e.g. 100)
    NKGRID=$(echo "$host_executable" | sed -n "s/.*_\([0-9]*\)k_${NKMGRID}km$/\1/p")
    # N. Kernels
    NKERNELS=$(echo "$host_executable" | sed -n "s/.*_\([0-9]*\)ker_${NKGRID}k_${NKMGRID}km$/\1/p")
    # N. Economies
    NMODELS=$(echo "$host_executable" | sed -n "s/\([0-9]*\)_${NKERNELS}ker_${NKGRID}k_${NKMGRID}km$/\1/p")
    
    echo "------------------------------------------------------"
    echo "STEP 1. Executing: HOST: ${host_executable} AFI: ${fpga_afi}"
    echo "(a) Individual capital grid size: $NKGRID"
    echo "(b) Aggregate capital grid size: $NKMGRID"
    echo "(c) Kernels: $NKERNELS"
    echo "(d) Economies: $NMODELS"
    echo "(e) F1 Instance: $INSTANCE"

    # Run the test for collecting power results
    $HOST_EXECUTABLES_DIR/$host_executable $FPGA_AFI_DIR/$fpga_afi.awsxclbin > "$RESULTS_DIR/$LOG_RESULTS_DIR/log_${host_executable}.txt"
    
    # Collect power results
    sudo fpga-describe-local-image -S 0 -M > "$RESULTS_DIR/power_${host_executable}_during.txt"
    
    # Run the test again to collect execution times and results 
    $HOST_EXECUTABLES_DIR/$host_executable $FPGA_AFI_DIR/$fpga_afi.awsxclbin > "$RESULTS_DIR/$LOG_RESULTS_DIR/log_${host_executable}.txt"

    # Extract: file with execution time
    file_with_path=$(find ./ -type f -name "fpga*-time-tot.txt" -print -quit)
    EXECUTIONTIMEFILE=$(basename "$file_with_path")
    echo "$EXECUTIONTIMEFILE"
    # Extract Device: 'fpgaI', 'fpgaII', 'fpgaIII'
    DEVICETYPE=$(echo "$EXECUTIONTIMEFILE" | sed 's/^\([^-\]*\)-.*/\1/')
    echo "$DEVICETYPE"  
    echo "(f) Device: $DEVICETYPE"
    echo "(g) Execution time file: $EXECUTIONTIMEFILE"

    echo "STEP 2. Copy files: HOST: ${host_executable} AFI: ${fpga_afi}"
    
    # Collect results
    if [ "$test_name" == "knl-1" ] || [ "$test_name" == "base-knl-1" ] || [ "$test_name" == "pip-knl-1" ]; then
        # -------------- Single-kernel
        # Execution time: fpgaI-base-knl-1-nKM4-nk100-time-tot
        TIMEFILENAME="${DEVICETYPE}-${test_name}-nKM${NKMGRID}-nk${NKGRID}"
        # 1st iteration final values to compare the precision across devices
        INFONAME="${DEVICETYPE}_${test_name}_nKM${NKMGRID}_nk${NKGRID}_i0_d0_k0_of_${NMODELS}"
        OHTERINFO="${DEVICETYPE}_${test_name}_nKM${NKMGRID}_nk${NKGRID}_${NMODELS}"
    else
        # -------------- Three-kernel (different FPGA Instances)
        # Execution time: fpgaII-nKM4-nk100-time-tot
        TIMEFILENAME="${DEVICETYPE}-nKM${NKMGRID}-nk${NKGRID}"
        # Rest of information: 1st iteration final values to compare the precision across devices
        INFONAME="${DEVICETYPE}_nKM${NKMGRID}_nk${NKGRID}_i0_d0_k0_of_${NMODELS}"
        OHTERINFO="${DEVICETYPE}_nKM${NKMGRID}_nk${NKGRID}_${NMODELS}"
    fi

    # Execution time
    mv ./fpga*-time-tot.txt "$RESULTS_DIR/${TIMEFILENAME}-time-tot.txt"
    mv ./fpga*-kernel-time.txt "$RESULTS_DIR/${TIMEFILENAME}-kernel-time.txt"
    mv ./fpga*-open-init-time.txt "$RESULTS_DIR/${TIMEFILENAME}-open-init-time.txt"
    mv ./fpga*-kernel-tot-time.txt "$RESULTS_DIR/${TIMEFILENAME}-kernel-tot-time.txt"
    mv ./fpga*-init-time.txt "$RESULTS_DIR/${TIMEFILENAME}-init-time.txt"
    mv ./fpga*-h2d-time.txt "$RESULTS_DIR/${TIMEFILENAME}-h2d-time.txt"
    mv ./fpga*-kernel-task-time.txt "$RESULTS_DIR/${TIMEFILENAME}-kernel-task-time.txt"
    mv ./fpga*-d2h-time.txt "$RESULTS_DIR/${TIMEFILENAME}-d2h-time.txt"
    mv ./fpga*-write-time.txt "$RESULTS_DIR/${TIMEFILENAME}-write-time.txt"
    # Coefficients
    mv "$RESULTS_DIR/$FINAL_VALUES_DIR/"coeffs_*i0_d0_k0.txt "$RESULTS_DIR/coeffs_${INFONAME}.txt"
    # kcross
    mv "$RESULTS_DIR/$FINAL_VALUES_DIR/"kcross_*i0_d0_k0.txt "$RESULTS_DIR/kcross_${INFONAME}.txt"
    # kprime
    mv "$RESULTS_DIR/$FINAL_VALUES_DIR/"kpo_*i0_d0_k0.txt "$RESULTS_DIR/kpo_${INFONAME}.txt"
    # r2
    mv "$RESULTS_DIR/$FINAL_VALUES_DIR/"r2bg_*i0_d0_k0.txt "$RESULTS_DIR/r2bg_${INFONAME}.txt"
    # egm_iter
    mv "$RESULTS_DIR/$FINAL_VALUES_DIR/"egm_iter_*i0_d0_k0.txt "$RESULTS_DIR/egm_iter_${INFONAME}.txt"
    # Power
    mv "$RESULTS_DIR/power_${host_executable}_during.txt" "$RESULTS_DIR/power_${INFONAME}.txt"
    # Collect FPGA run summaries
    mv device_trace*.csv "$RESULTS_DIR/device_trace_${OHTERINFO}.csv"
    mv opencl_trace*.csv "$RESULTS_DIR/opencl_trace_${OHTERINFO}.csv"
    mv summary*.csv "$RESULTS_DIR/summary_${OHTERINFO}.csv"
    mv xrt*.run_summary "$RESULTS_DIR/xrt_${OHTERINFO}.run_summary"
    # Collect reports and logs
    mv $RESULTS_DIR/$LOG_RESULTS_DIR/log_${host_executable}.txt "$RESULTS_DIR/log_${OHTERINFO}.txt"
    
    echo "Completed: HOST: ${host_executable} AFI: ${fpga_afi}"
    echo "------------------------------------------------------"
    
    # Wait 10 seconds before running the next test
    sleep 10   
}

# Table 3: Function to run test cases for a given number of economies across different FPGA f1 instances: 2x, 4x, 16x
run_table3_tests() {
    local num_econ=$1
    # 1 case: The function run_test_case receives three arguments: 
    # a. ${num_econ}_3ker_100k_4km: ouptut txt name convention
    # b. ${num_econ}_3ker_100k_4km: host_executable name
    # c. ${num_econ}_3ker_100k_4km: fpga_afi name
    run_test_case "${num_econ}_3ker_100k_4km" "${num_econ}_3ker_100k_4km" "3ker_100k_4km"
}

# Table 4: Function to run test cases for a given number of economies on f1.2xlarge instance
run_table4_tests() {
    local num_econ=$1
    # Single-kernel design across different grid sizes
    # Grid Sizes: 100k, 200k, 300k x 4km
    run_test_case "knl-1" "${num_econ}_1ker_100k_4km" "1ker_100k_4km"
    run_test_case "knl-1" "${num_econ}_1ker_200k_4km" "1ker_200k_4km"
    run_test_case "knl-1" "${num_econ}_1ker_300k_4km" "1ker_300k_4km"
    # Grid Sizes: 100k, 200k, 300k x 8km
    run_test_case "knl-1" "${num_econ}_1ker_100k_8km" "1ker_100k_8km"
    run_test_case "knl-1" "${num_econ}_1ker_200k_8km" "1ker_200k_8km"
    run_test_case "knl-1" "${num_econ}_1ker_300k_8km" "1ker_300k_8km"
}

# Function to run acceleration test cases for a given number of economies
run_table5_tests() {
    local num_econ=$1
    # Acceleration tests using 100k, 4km grid
    run_test_case "base-knl-1" "${num_econ}_1ker_100k_4km" "baseline_1ker_100k_4km"
    run_test_case "pip-knl-1" "${num_econ}_1ker_100k_4km" "pipeline_1ker_100k_4km"
}

##Start of the script

# Restart MPD
sudo systemctl restart mpd
sleep 5

# Source FPGA setup files
AWS_FPGA_REPO_DIR="/home/centos/src/project_data/aws-fpga"

# Check if the directory already exists
if [ -d "$AWS_FPGA_REPO_DIR" ]; then
    echo "Directory already exists. Skipping clone."
else
    # Clone the repository if the directory doesn't exist
    git clone https://github.com/aws/aws-fpga.git "$AWS_FPGA_REPO_DIR"
fi

. $AWS_FPGA_REPO_DIR/vitis_setup.sh
. $AWS_FPGA_REPO_DIR/vitis_runtime_setup.sh
export PLATFORM_REPO_PATHS=$(dirname $AWS_PLATFORM)

# Set execute permissions for host executables
chmod +x $HOST_EXECUTABLES_DIR/*

echo "*******************************************************************************************************"
echo "********** NOTE: RESTART FPGA INSTANCE IF YOU CANCEL THE SCRIPT WHILE IN PROGRESS**********************"
echo "On-going logs can be found in {$RESULTS_DIR/$LOG_RESULTS_DIR}"
echo "Estimated time to complete table3, table4, table5 on f1.2x = xx, xx, xx hrs respectively." 
echo "*******************************************************************************************************"

if [[ "$TABLE" == "3" ]]; then
    echo "Executing command for generating Table3 results. Function receives as input number of economies NUM_ECON_TABLE_3"
    run_table3_tests "$NUM_ECON_TABLE_3"

elif [[ "$TABLE" == "4" ]]; then
    echo "Executing command for generating table4 results. Function receives as input number of economies NUM_ECON_TABLE_4"
    run_table4_tests "$NUM_ECON_TABLE_4"

elif [[ "$TABLE" == "5" ]]; then
    echo "Executing command for generating table5 results. Function receives as input number of economies NUM_ECON_TABLE_5"
    run_table5_tests "$NUM_ECON_TABLE_5"

elif [[ "$TABLE" == "all" ]]; then
    echo "Executing command for generating all tables results"
    echo "Starting table 3 results generation"
    run_table3_tests "$NUM_ECON_TABLE_3"
    echo "Starting table 4 results generation"
    run_table4_tests "$NUM_ECON_TABLE_4"
    echo "Starting table 5 results generation"
    run_table5_tests "$NUM_ECON_TABLE_5"

else
    echo "Error: Argument is not one among the list."
    echo "Allowed arguments for TABLE are: 3, 4, 5, or 'all'"
    exit 1
fi

# Upload text files from RESULTS_DIR to the S3 bucket
for file in "$RESULTS_DIR"/*.txt "$RESULTS_DIR"/*.csv "$RESULTS_DIR"/*.run_summary; do
    aws s3 cp "$file" "s3://$S3_EXE_BUCKET_NAME/$S3_RESULTS_DIR/"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to copy file '$file' to S3 bucket."
        exit 1
    else
        echo "File '$file' copied successfully."
    fi
done