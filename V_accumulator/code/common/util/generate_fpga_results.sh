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

# AWS S3 bucket to store the final timing results
S3_EXE_BUCKET_NAME="fpga-econ-acc"
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

    EXERCISE=$(echo "$fpga_afi" | sed 's/afi_//')

    echo "------------------------------------------------------"
    echo "STEP 1. Executing: HOST: ${host_executable} AFI: ${fpga_afi}"
    echo "(a) F1 Instance: $INSTANCE"
    echo "(b) FPGA image: $EXERCISE"

    # Run the test for collecting power results
    $HOST_EXECUTABLES_DIR/$host_executable $FPGA_AFI_DIR/$fpga_afi.awsxclbin > "$RESULTS_DIR/$LOG_RESULTS_DIR/log_${host_executable}.txt"
    
    # Collect power results
    sudo fpga-describe-local-image -S 0 -M > "$RESULTS_DIR/power_${EXERCISE}_during.txt"
    
    # Run the test again to collect execution times and results 
    $HOST_EXECUTABLES_DIR/$host_executable $FPGA_AFI_DIR/$fpga_afi.awsxclbin > "$RESULTS_DIR/$LOG_RESULTS_DIR/log_${EXERCISE}.txt"

    echo "STEP 2. Copy files: HOST: ${host_executable} AFI: ${fpga_afi}"
    
    # Execution time
    mv ${RESULTS_DIR}/exec_time_*.txt ${RESULTS_DIR}/exec_time-${EXERCISE}-time-tot.txt
    # Results
    mv ${RESULTS_DIR}/final_values/sum_*.txt ${RESULTS_DIR}/sum_fpga_${EXERCISE}.txt
    # Power
    mv ${RESULTS_DIR}/power_${EXERCISE}_during.txt ${RESULTS_DIR}/power_${EXERCISE}.txt
    # Collect FPGA run summaries
    mv device_trace*.csv "$RESULTS_DIR/device_trace_${EXERCISE}.csv"
    mv opencl_trace*.csv "$RESULTS_DIR/opencl_trace_${EXERCISE}.csv"
    mv summary*.csv "$RESULTS_DIR/summary_${EXERCISE}.csv"
    mv xrt*.run_summary "$RESULTS_DIR/xrt_${EXERCISE}.run_summary"
    
    echo "Completed: HOST: ${host_executable} AFI: ${fpga_afi}"
    echo "------------------------------------------------------"
    
    # Wait 10 seconds before running the next test
    sleep 10   
}

# Table 3: Function to run test cases for a given number of economies across different FPGA f1 instances: 2x, 4x, 16x
run_table_all_tests() {
    local num_econ=$1
    # 1 case: The function run_test_case receives three arguments: 
    # a. test_name: name of the test
    # b. host_executable: host_executable name (e.g. host_base)
    # c. fpga_afi: fpga_afi name (e.g. afi_base)
    run_test_case "base" "host_base" "afi_base"
    run_test_case "opt" "host" "afi_optimized"
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
echo "*******************************************************************************************************"

if [[ "$TABLE" == "ALL" ]]; then
    echo "Executing two images. (1) base without acceleration; (2) optimized"
    # 1 case: The function run_test_case receives three arguments: 
    # a. test_name: name of the test
    # b. host_executable: host_executable name (e.g. host_base)
    # c. fpga_afi: fpga_afi name (e.g. afi_base)
    run_test_case "base" "host_base" "afi_base"
    run_test_case "opt" "host" "afi_optimized"
elif [[ "$TABLE" == "BASE" ]]; then
    echo "Executing base image with no acceleration."
    # 1 case: The function run_test_case receives three arguments: 
    # a. test_name: name of the test
    # b. host_executable: host_executable name (e.g. host_base)
    # c. fpga_afi: fpga_afi name (e.g. afi_base)
    run_test_case "base" "host_base" "afi_base"
elif [[ "$TABLE" == "OPT" ]]; then
    echo "Executing optimized image."
    # 1 case: The function run_test_case receives three arguments: 
    # a. test_name: name of the test
    # b. host_executable: host_executable name (e.g. host_base)
    # c. fpga_afi: fpga_afi name (e.g. afi_base)
    run_test_case "opt" "host" "afi_optimized"
else
    echo "Error: Argument is not one among the list."
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