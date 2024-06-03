#!/bin/bash
BASE=$(pwd)
# Automatically collect number of cores in the m5n instance
CORES_PER_SOCKET=$(lscpu | grep 'Core(s) per socket:' | awk '{print $4}')
SOCKETS=$(lscpu | grep 'Socket(s):' | awk '{print $2}')

# Calculate total number of CPU cores
CPU_CORES=$((CORES_PER_SOCKET * SOCKETS))
echo "Number of CPU CORES: $CPU_CORES"

# Number of Economies to run in Table 2 and 3 of the paper
NUM_ECON_TABLE_2=1200 # default value = 1200
NUM_ECON_TABLE_3=1200 # default value = 1200

# AWS S3 bucket to store the final timing results
S3_EXE_BUCKET_NAME="fpga-econ-ks"
S3_CPU_DIR="executables/cpu"
S3_RESULTS_DIR="results/cpu"
AWS_REGION="us-west-2"

# Result directories on the m5n instance
RESULTS_DIR="results/cpu"
FINAL_VALUES_DIR="final_values"
LOG_RESULTS_DIR="log_results"

# Executables directories on the m5n instance
EXECUTABLES_DIR="executables/cpu"

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
    create_directories "$EXECUTABLES_DIR"
    aws s3 cp --recursive "s3://$S3_EXE_BUCKET_NAME/$S3_CPU_DIR/" $EXECUTABLES_DIR/
    if [ $? -ne 0 ]; then
        echo "Error: Failed to copy executables from S3 bucket."
        exit 1
    else
        echo "Executables copied successfully from S3 bucket."
    fi
else
    echo "Using the pre-compiled executables from the repository"
fi

# Wait 5 seconds to let the user see the log
sleep 5

# Set the openmpi env (This should be before the openmpi check/installation)
export PATH=$PATH:$HOME/openmpi/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/openmpi/lib

# Check if OpenMPI is already installed
if ! command -v mpic++ &> /dev/null; then
    echo "OpenMPI not found. Installing..."
    cd common/util/
    sh ./OpenMPI_install.sh
    cd ../../
    echo "OpenMPI installation complete."
else
    echo "OpenMPI is already installed. Skipping installation."
fi

# Function to run a test case
run_test_case() {
    local program_name=$1       # openmpi executable name
    local grid_name=$2          # output file txt name
    # Create directories using the name provided as input
    create_directories "$RESULTS_DIR/$FINAL_VALUES_DIR"
    create_directories "$RESULTS_DIR/$LOG_RESULTS_DIR"

    local log_file="$RESULTS_DIR/$LOG_RESULTS_DIR/log_$program_name.txt"

    if [ "$CPU_CORES" -eq 1 ]; then
        echo "Executing $program_name: Sequential Execution"
        ./$EXECUTABLES_DIR/$program_name > "$log_file"
    else
        echo "Executing $program_name: OpenMPI"
        mpirun -n $CPU_CORES $EXECUTABLES_DIR/$program_name > "$log_file"
    fi
    echo "Completed ${program_name}"

    # Rename required outside 
    if [ "$grid_name" == "linear" ] || [ "$grid_name" == "binary" ]; then
    mv ./cpu-cores*-time-tot.txt "$RESULTS_DIR/cpu-cores${CPU_CORES}-${grid_name}-time-tot.txt"
    mv ./cpu-cores*-kernel-time.txt "$RESULTS_DIR/cpu-cores${CPU_CORES}-${grid_name}-kernel-time.txt"
    mv ./cpu-cores*-init-time.txt "$RESULTS_DIR/cpu-cores${CPU_CORES}-${grid_name}-init-time.txt"
    mv ./cpu-cores*-write-time.txt "$RESULTS_DIR/cpu-cores${CPU_CORES}-${grid_name}-write-time.txt"
    else
    mv ./cpu-cores*.txt "$RESULTS_DIR/"
    fi
}

# Function to run ALL model specifications for a given number of economies on the m5n.large instance
run_m5n_all() {
    local num_econ=$1
    # Acceleration tests using 100k, 4km grid
    run_test_case "${num_econ}_linear" "linear"
    run_test_case "${num_econ}_binary" "binary"
    # 3 cases - 100k, 200k, 300k x 4km
    run_test_case "${num_econ}_100k_4km" "100k_4km"
    run_test_case "${num_econ}_200k_4km" "200k_4km"
    run_test_case "${num_econ}_300k_4km" "300k_4km"
    # 3 cases - 100k, 200k, 300k x 8km
    run_test_case "${num_econ}_100k_8km" "100k_8km"
    run_test_case "${num_econ}_200k_8km" "200k_8km"
    run_test_case "${num_econ}_300k_8km" "300k_8km"
}

run_m5n_batch1() {
    local num_econ=$1
    # Acceleration tests using 100k, 4km grid
    run_test_case "${num_econ}_linear" "linear"
    run_test_case "${num_econ}_binary" "binary"
    # 3 cases - 100k, 200k, 300k x 4km
    run_test_case "${num_econ}_100k_4km" "100k_4km"
    run_test_case "${num_econ}_200k_4km" "200k_4km"
    run_test_case "${num_econ}_300k_4km" "300k_4km"
}

run_m5n_batch2() {
    local num_econ=$1
    run_test_case "${num_econ}_100k_8km" "100k_8km"
    run_test_case "${num_econ}_200k_8km" "200k_8km"
}

run_m5n_batch3() {
    local num_econ=$1
    run_test_case "${num_econ}_300k_8km" "300k_8km"
}

# Function to run benchmark model on larger instances
run_m5n_large_tests() {
    local num_econ=$1
    # 100k x 4km
    run_test_case "mpi_${num_econ}_100k_4km" "100k_4km"
}


# Set execute permissions for host executables
chmod +x $EXECUTABLES_DIR/*

# Run all grid points test cases
echo "*******************************************************************************************************"
echo "Benchmarke model: Estimated time to complete ${NUM_ECON_TABLE_3} economies on m5n.large in 10 hours"
echo "On-going logs can be found in {$RESULTS_DIR/$LOG_RESULTS_DIR}"
echo "*******************************************************************************************************"

# Run acceleration test cases to show the speedup of search algorithms
echo "m5n instance with ${CPU_CORES} core(s)."
if [[ "$M5N" == "1x" ]]; then
    echo "Executing commands to generate results for all model specifications. NUM_ECON_TABLE_2 selects the appropriate binary."
    run_m5n_all "$NUM_ECON_TABLE_2"
elif [[ "$M5N" == "1xBATCH1" ]]; then
    echo "Executing commands to generate results for: linear, binary, 100-4, 200-4, 300-4. NUM_ECON_TABLE_2 selects the appropriate binary."
    run_m5n_batch1 "$NUM_ECON_TABLE_2"
elif [[ "$M5N" == "1xBATCH2" ]]; then
    echo "Executing commands to generate results for: 100-8, 200-8. NUM_ECON_TABLE_2 selects the appropriate binary."
    run_m5n_batch2 "$NUM_ECON_TABLE_2"    
elif [[ "$M5N" == "1xBATCH3" ]]; then
    echo "Executing commands to generate results for: 300-8. NUM_ECON_TABLE_2 selects the appropriate binary."
    run_m5n_batch3 "$NUM_ECON_TABLE_2"    
elif [[ "$M5N" == "4x" ]] || [[ "$M5N" == "24x" ]]; then
    echo "Executing commands to generate results for benchmark model. NUM_ECON_TABLE_3 selects the appropriate binary."
    run_m5n_large_tests "$NUM_ECON_TABLE_3"
else
    echo "Error: M5N instance is not one among the list."
    echo "Allowed arguments for M5N are: 1x, 4x, or 24x"
    exit 1
fi

############################ MOVING RESULTS TO S3 BUCKET
# Copy 1st iteration final values (coefficients, kcross, kprime, r2, ihp iter) to compare the precision across devices
cp -r "$RESULTS_DIR/$FINAL_VALUES_DIR/"*i0_of*.txt "$RESULTS_DIR/"

# Upload text files from RESULTS_DIR to the S3 bucket
for file in "$RESULTS_DIR"/*.txt; do
    aws s3 cp "$file" "s3://$S3_EXE_BUCKET_NAME/$S3_RESULTS_DIR/"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to copy file '$file' to S3 bucket."
        exit 1
    else
        echo "File '$file' copied successfully."
    fi
done