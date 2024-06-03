# Programming FPGAs for Economics: <br> An Introduction to Electrical Engineering Economics Replication Package README 

### Authors
- **Bhagath Cheela**  
  Department of Electrical and Systems Engineering, University of Pennsylvania  
  [cheelabhagath@gmail.com](mailto:cheelabhagath@gmail.com)

- **André DeHon**  
  Department of Electrical and Systems Engineering, University of Pennsylvania  
  [andre@acm.org](mailto:andre@acm.org)

- **Jesús Fernández-Villaverde**  
  Department of Economics, University of Pennsylvania  
  [jesusfv@econ.upenn.edu](mailto:jesusfv@econ.upenn.edu)

- **Alessandro Peri**  
  Department of Economics, University of Colorado, Boulder  
  [alessandro.peri@colorado.edu](mailto:alessandro.peri@colorado.edu)


## 1 Introduction

This document serves as the README file for replicating the results presented in the paper "Programming FPGAs for Economics: An Introduction to Electrical Engineering Economic", by Bhagath Cheela, André DeHon, Jesús Fernández-Villaverde and Alessandro Peri.

This document focuses on the steps required for replication. Interested readers are invited to explore our comprehensive tutorial Cheela et al. (2023) for a thorough examination of the content within each file. The analysis is performed using the cloud services provided by Amazon Web Services (AWS). For learning how to create an AWS account follow this link. For learning how to launch an AWS instances follow this link.

## 2 Directory Structure

The replication directory is structured as follows.

```
./code
./documents
./I_estimation_results 
./II_post_analysis
./III_floats 
./IV_paper
```

In particular:

- ./code: contains the code used to estimate the Krusell and Smith (1998) model;
- ./documents: contains our tutorial Cheela et al. (2023) and supporting documents with essential information for replicating our FPGA results;
- ./I_estimation_results: here, you can access the output generated from model estimation across alternative software-devices;
- ./II_post_analysis: contains the script main.m that generates all tables and floats for the paper. Results are stored in ./III_floats and ./IV_paper/results;
- ./III_floats: this folder contains the binary file with all tables in the paper (that require model estimates);
- ./IV_paper/results: houses the .txt files created during the post-analysis, which are later incorporated into the paper's $\mathrm{Latex}$ file.


## 3 Computing the Model across Software-Devices

### 3.1 GPU

Folder: ./code/gpu

Description: To generate the model estimates on the GPU using Numba Cuda compiler on an NVIDIA GPU (A100, in our work) follows this step:

- Copy the agshock.txt and idshock.txt files from the directory ./code/common/shocks/ and paste them into ./code/gpu/input
- Run on the terminal: python3 ks_gpu.py > output
- Move the output file to ./I_estimation_results/gpu

Remark: This analysis requires accesss to an NVIDIA GPU.

### 3.2 CPU-Matlab

Folder. ./code/matlab/.

Description. To generate the model estimates on the CPU using Matlab run: master.m. This script calls the function MMV func.m, which modifies the original Maliar et al. (2010)'s script MAIN.m, to automatically produce model estimates for alternative grid sizes.

Output. The resulting model estimates are saved in binary files located in the folder ./I_estimation_results/matlab/MMV/.

Remark. Differently from the original Maliar et al. (2010)'s code, we replaced the spline interpolation with the linear interpolation. In addition, to guarantee replication across different software and devices, we need to fix aggregate and idiosyncratic shocks. To do so: (i) we set the Mersenne Twister random number generator's seed to 1 in ./code/matlab/SHOCK.m to fix aggregate and idiosyncratic shocks; (ii) we add lines 103-104 MMV_func.m to store the shocks in the folder ./code/common/shocks/. Note: these lines are commented out as we need to store the shocks only once.

### 3.3 CPU-C

Folders: ./code/common and ./code/fpga

Description. To implement the Krusell and Smith (1998)'s algorithm on the CPU:

1. Compile all the binaries.
2. Execute the binaries on AWS.

### 3.3.1 Compile all the binaries

1. Launch the Instance. Log into the AWS instance m5n.large. To set up and launch the instance, follow the instructions in documents/CPU-run.pdf.
2. Install the Packages. Initiate a terminal session on the AWS instance and run the subsequent script to install the utilities git, make, tmux and wget:
```
sudo yum install git -y
sudo yum install make -y
sudo yum install tmux -y
sudo yum install wget -y
```

3. Clone the GitHub repositories. Clone our GitHub repository into a directory of your preference (e.g., /home/ec2-user):
```
git clone https://github.com/AleP83/FPGA-Econ.git
```
1. Set the AWS credentials. Configure your AWS credentials by executing the following command in the terminal:
    ```
    aws configure
    ```
    Follow the steps here:
    ```
    $ aws configure
    AWS Access Key ID [*************xxxx]: <Your AWS Access Key ID>
    AWS Secret Access Key [**************xxxx]: <Your AWS Secret Access
        Key>
    Default region name: us-west-2
    Default output format [None]: json
    ```

    For more information visit this [link](https://wellarchitectedlabs.com/common/documentation/aws_credentials/).

5. Install OpenMPI. Run the following script from the terminal:
    ```
    sh code/common/util/OpenMPI_install.sh
    ```
    Note: Installing Open-MPI may take some time (10-15 minutes).

6. Set the OpenMPI environment. If you are compiling or building for parallel execution, execute the following commands in the terminal from the parent directory:
    ```
    export PATH=$PATH:$HOME/openmpi/bin
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/openmpi/lib
    ```
7. Modify the Makefile. Update settings in the code/Makefile as follows:

   - Set the AWS S3 Bucket Name: Specify the S3 bucket name by replacing S3-NAME-GOES-HERE
       ```
       S3_EXE_BUCKET_NAME := S3-NAME-GOES-HERE
       ```
       Remark: The S3 bucket name must be globally unique within AWS. If an error occurs during bucket creation, it may be due to the name being already in use by another user.

   - Select the AWS region of the S3 bucket (default is us-west-2):
        ```
        AWS_REGION := us-west-2
        ```
1. Modify the Main. Open /code/common/app.cpp and set the number of models N_MODEL you want to compute ( 1,200 in our benchmark specification):
    ```
    #define N_MODEL 1200 // total number of models
    ```
9. Set the Grid Sizes. Open /code/common/definitions.h and set the grid sizes:
    ```
    #define NKGRID 100 // grid points on individual capital grid
    #define NKM_GRID 4 // grid points on aggregate capital grid
    ```
    The benchmark code is set to allocate NKGRID=100, NKM_GRID=4.

10. Set the Software Design. Open/code/common/dev_options.h and select the interpolation-range search algorithm:
    ```
    // Set only one of the following macros to 1, keeping the rest to
        zero.
    #define _LINEAR_SEARCH 0
    #define _BINARY_SEARCH 0
    #define _CUSTOM_BINARY_SEARCH 1
    ```
    The benchmark code is set to implement the jump-search algorithm _CUSTOM_BINARY_SEARCH 1.

11. Compile the binary. After modifying the files, navigate to the /code directory using the terminal. Then, compile the application for CPU execution using the following command:

    - For building binaries for sequential execution on single-core instance:
    ```
    make cpu_to_s3 CPU_EXE=<# Economies>_<#indiv cap.>_<#agg cap.>
    ```
    For example, compile the benchmark model as follows:
    ```
    make cpu_to_s3 CPU_EXE=1200_100k_4km
    ```
    - For building binaries for parallel execution on multi-core instance:
    ```
    make openmpi_to_s3 OPENMPI_EXE=mpi_<# Economies>_<#indiv capital>_<#agg capital>
    ```
    For example, compile the benchmark model as follows:
    ```
    make openmpi_to_s3 OPENMPI_EXE=mpi_1200_100k_4km
    ```
1.  Compile all binaries. To streamline the process, Table 1-9 in Appendix A concisely summarizes all manual changes to the code required to compile binaries for all of the combinations, NKGRID $\in\{100,200,300\}$, NKM_GRID $\in\{4,8\}$, and search algorithms $\in\{$ linear,binary,custom_binary $\}$, required to replicate the results in the paper.
    ```
    make cpu_to_s3 CPU_EXE=1200_100k_4km
    make cpu_to_s3 CPU_EXE=1200_200k_4km
    make cpu_to_s3 CPU_EXE=1200_300k_4km
    make cpu_to_s3 CPU_EXE=1200_100k_8km
    make cpu_to_s3 CPU_EXE=1200_200k_8km
    make cpu_to_s3 CPU_EXE=1200_300k_8km
    make cpu_to_s3 CPU_EXE=1200_linear
    make cpu_to_s3 CPU_EXE=1200_binary
    make openmpi_to_s3 OPENMPI_EXE=mpi_1200_100k_4km
    ```

Output: The make cpu_to_s3 and make openmpi_to_s3 commands will save the binaries in your S3 bucket, identified as \$S3_EXE_BUCKET_NAME, under the folder s3://\$S3_EXE_BUCKET_NAME/executables/cesu/:

```
$S3_EXE_BUCKET_NAME/
    executables/
        cpu/
            1200_100k_4km
            1200_200k_4km
            1200_300k_4km
            1200_100k_8km
            1200_200k_8km
            1200_300k_8km
            1200_linear
            1200_binary
            mpi_1200_100k_4km
```


### 3.3.2 Execute the binaries on AWS

1. Launch the Instance. Log into the appropriate AWS instance: m5n.large, m5n.4xlarge, or m5n.24xlarge. To set up and launch the instance, follow the instructions in documents/CPU-run.pdf.
2. Install the Packages. Initiate a terminal session on the AWS instance and run the subsequent script to install the utilities git, make, tmux and wget:
    ```
    sudo yum install git -y
    sudo yum install make -y
    sudo yum install tmux -y
    sudo yum install wget -y
    ```

3. Clone the GitHub repositories. Clone our GitHub repository into a directory of your preference (e.g. /home/ec2-user):
    ```
    git clone https://github.com/AleP83/FPGA-Econ.git
    ```

4. Set the AWS credentials. Configure your AWS credentials by executing the following command in the terminal:
    ```
    aws configure
    ```
    Follow the steps here:
    ```
    $ aws configure
    AWS Access Key ID [*************xxxx]: <Your AWS Access Key ID>
    AWS Secret Access Key [**************xxxx]: <Your AWS Secret Access
        Key>
    Default region name: us-west-2
    Default output format [None]: json
    ```

    For more information visit this [link](https://wellarchitectedlabs.com/common/documentation/aws_credentials/).


5. Modify the Makefile. Update settings in the code/Makefile as follows:
   - Set the AWS S3 Bucket Name: Specify the S3 bucket name by replacing S3-NAME-GOES-HERE
       ```
       S3_EXE_BUCKET_NAME := S3-NAME-GOES-HERE
       ```
       Remark: The S3 bucket name must be globally unique within AWS. If an error occurs during bucket creation, it may be due to the name being already in use by another user.

   - Select the AWS region of the S3 bucket (default is us-west-2):
        ```
        AWS_REGION := us-west-2
        ```
1. Modify Shell Script for CPU Results. Update settings in the code/common/util/generate_cpu_results.sh as follows:
   - Set the AWS S3 Bucket Name: Specify the S3 bucket name by replacing S3-NAME-GOES-HERE
    ```
    S3_EXE_BUCKET_NAME="S3-NAME-GOES-HERE"
    ```
    Remark: The S3 bucket name must be globally unique within AWS. If an error occurs during bucket creation, it may be due to the name being already in use by another user.

   - Select the AWS region of the S3 bucket (default is us-west-2):
    ```
    AWS_REGION="us-west-2"
    ```
    AWS Region and Bucket name should coincide with the ones used in the compiling stage.

1. Initiate tmux terminal session. To ensure your terminal session remains active throughout the potentially lengthy execution, initiate a terminal multiplexer session:
    ```
    tmux
    ```
    The tmux command allows you to detach and reattach to terminal sessions without interruption. For example, to resume a tmux session with index 0 , use the following command:
    ```
    tmux attach -t 0
    ```
    For detailed instructions on how to use tmux, see this [guide](https://hamvocke.com/blog/a-quick-and-easy-guide-to-tmux/).

8. Run all binaries. To run the binaries on the CPU, navigate to the directory /code from within the tmux terminal window. Therein, execute the binaries sequentially and copy the generated results to AWS-S3 Bucket with the following AWS instance specific commands:
    - To replicate results on the m5n.large instance execute on the tmux terminal:
    ```
    make cpu_results M5N=1x USE_AWS_S3_EXE=yes
    ```
    Execution time: This step takes about one week (approximately six and a half days). To expedite the process (down to 50 hours), we provide commands to split the workload into three batches. These batches can be executed concurrently on three distinct m5n.large instances. This is achieved by replacing M5N=1x in the command with $\mathrm{M} 5 \mathrm{~N}=1 \mathrm{xBATCH} 1, \mathrm{M} 5 \mathrm{~N}=1 \mathrm{xBATCH} 2$, and $\mathrm{M} 5 \mathrm{~N}=1 \mathrm{xBATCH} 3$. For instance, to initiate the first batch, the following command can be used:
    ```
    make cpu_results M5N=1xBATCH1 USE_AWS_S3_EXE=yes
    ```
    - To replicate results on the m5n.4xlarge instance execute on the tmux terminal:
    ```
    make cpu_results M5N=4x USE_AWS_S3_EXE=yes
    ```
    Execution time: About one hour.

    - To replicate results on the m5n.24xlarge instance execute on the tmux terminal: 
    ```
    make cpu_results M5N=24x USE_AWS_S3_EXE=yes
    ```
    Execution time: About 10 minutes.

Output. The command make cpu_results automatically saves the results in your S3 bucket, identified as \$S3_EXE_BUCKET_NAME, under the folder s3://\$S3_EXE_BUCKET_NAME/results/cpu/:

```
$S3_EXE_BUCKET_NAME/
    results/
        cpu/
            *.txt
```


### 3.4 FPGA

Folders: ./code/common and ./code/fpga

Description. To implement the Krusell and Smith (1998) algorithm on the FPGA:

1. Synthesize the application in hardware.
2. Execute the application on an FPGA instance.

### 3.4.1 Synthesize the application in hardware

The replication package provides the pre-synthesized images in the directory ./code/executables/fpga/fpga_afi and associated host binaries in ./code/executables/fpga/host_executables. To create these images follow these steps:

1. Launch the Instance. Log into the AWS build instance: z1d.2xlarge. To set up and launch the instance, follow the instructions in documents/FPGA-design.pdf.
2. Clone the GitHub repositories. Open the terminal. Then, clone the AWS repository and our GitHub repository into a directory of your preference (e.g., /home/centos/):
    ```
    git clone https://github.com/aws/aws-fpga.git \$AWS_FPGA_REPO_DIR
    git clone https://github.com/AleP83/FPGA-Econ.git
    ```
3. Set the AWS credentials. Configure your AWS credentials by executing the following command in the terminal:
    ```
    aws configure
    ```
    Follow the steps here:

    ```
    $ aws configure
    AWS Access Key ID [*************xxxx]: <Your AWS Access Key ID>
    AWS Secret Access Key [**************xxxx]: <Your AWS Secret Access
        Key>
    Default region name: us-west-2
    Default output format [None]: json
    ```
    For more information visit this [link](https://wellarchitectedlabs.com/common/documentation/aws_credentials/).


4. Modify the Makefile. Update settings in the code/Makefile as follows:
   - Set the AWS S3 Bucket Name: Specify the S3 bucket name by replacing S3-NAME-GOES-HERE
   ```
   S3_EXE_BUCKET_NAME := S3-NAME-GOES-HERE
   ```
   Remark: The S3 bucket name must be globally unique within AWS. If an error occurs during bucket creation, it may be due to the name being already in use by another user.
   - Select the AWS region of the S3 bucket (default is us-west-2):
   ```
   AWS_REGION := us-west-2
   ```
5. Modify the Main. Open /code/common/app.cpp and set the number of models N_MODEL you want to compute ( 1,200 in our benchmark specification):
    ```
    #define N_MODEL 1200 // total number of models
    ```

1. Set the Grid Sizes. Open/code/common/definitions.h and set the grid size:
    ```
    #define NKGRID 100 // grid points on individual capital grid
    #define NKM_GRID 4 // grid points on aggregate capital grid
    ```

1. Set the Hardware Design. Open /code/common/dev_options.h and select the FPGA design:
    ```
    #define _BASELINE 0 // Design with no HLS acceleration.
    #define _PIPELINE 0 // Design with only PIPELINE acceleration
    #define _WITHIN_ECONOMY 0 // Single-Kernel Design
    #define _ACROSS_ECONOMY 1 // Three-kernel Design (Benchmark)
    ```

1. Set the Hardware Design Specs. Open/code/fpga/design.cfg and select the single vs three-kernel design by appropriately commenting out the code you do not need. For example, the listing below executes the three-kernel design by commenting out (using $\#)$ the one-kernel design:
    ```
    # Enable either single kernel or three kernel
    [connectivity]
    ###############################################################################
    # nk=runOnfpga:1:runOnfpga_1
    ###############################################################################
    nk=runOnfpga:3:runOnfpga_1.runOnfpga_2.runOnfpga_3
    slr=runOnfpga_1:SLR2
    slr=runOnfpga_2:SLR1
    slr=runOnfpga_3:SLR0
    sp=runOnfpga_1.m_axi_gmem0:DDR[1]
    sp=runOnfpga_2.m_axi_gmem0:DDR[0]
    sp=runOnfpga_3.m_axi_gmem0:DDR[3]
    ```

1. Create all FPGA images. To ensure your terminal session remains active throughout the potentially lengthy synthesis process, initiate a terminal multiplexer session:
    ```
    tmux
    ```
    The tmux command allows you to detach and reattach to terminal sessions without interruption. For example, to resume a tmux session with index 0 , use the following command:
    ```
    tmux attach -t 0
    ```
    For detailed instructions on how to use tmux, see this [guide](https://hamvocke.com/blog/a-quick-and-easy-guide-to-tmux/).

    To initiate the synthesis of the FPGA circuit, navigate to the directory /code from within the tmux terminal window. Therein, execute the following instructions to generate the host and fpga target files on the build instance (z1d.2xlarge); and subsequently, upload the resulting executables to the AWS bucket:

    ```
    make clean
    unset XCL_EMULATION_MODE
    //setup environment
    source $AWS_FPGA_REPO_DIR/vitis_setup.sh
    export PLATFORM_REPO_PATHS=$(dirname $AWS_PLATFORM)
    export XCL_EMULATION_MODE=hw
    //build the target
    make afi FPGA_BIN=<fpga_bin> HOST_BIN=<host_bin>
    #E.g. make afi FPGA_BIN=3ker_100k_4km HOST_BIN=1200_3ker_100k_4km
    ```

    In particular, follow this fpga_bin-host_bin naming convention:

    ```
    make afi FPGA_BIN=3ker_100k_4km HOST_BIN=1200_3ker_100k_4km
    make afi FPGA_BIN=1ker_100k_4km HOST_BIN=1200_1ker_100k_4km
    make afi FPGA_BIN=1ker_200k_4km HOST_BIN=1200_1ker_200k_4km
    make afi FPGA_BIN=1ker_300k_4km HOST_BIN=1200_1ker_300k_4km
    make afi FPGA_BIN=1ker_100k_8km HOST_BIN=1200_1ker_100k_8km
    make afi FPGA_BIN=1ker_200k_8km HOST_BIN=1200_1ker_200k_8km
    make afi FPGA_BIN=1ker_300k_8km HOST_BIN=1200_1ker_300k_8km
    make afi FPGA_BIN=baseline_1ker_100k_4km HOST_BIN=120_1ker_100k_4km
    make afi FPGA_BIN=pipeline_1ker_100k_4km HOST_BIN=120_1ker_100k_4km
    ```

    Table 10-18 in Appendix B concisely summarize all manual changes to the code required to synthesize all nine FPGA images used in the paper.

    Estimated Run Time. The estimated time to build the three-kernel design is 8 hours. Single kernel design take on average less than 4 hours each.

Output: The command make afi automatically saves FPGA images and host binaries in your S3 bucket, identified as \$S3_EXE_BUCKET_NAME. This process organizes the files in the folder s3://\$S3_EXE_BUCKET_NAME/executables/fpga/ as follows:

- ./fpga_afi/<fpga_bin>: stores the FPGA images
- ./host_executables/<host_bin>: stores the host binaries that call the FPGA images

```
$S3_EXE_BUCKET_NAME/
    executables/
        fpga/
            fpga_afi/
                1ker_100k_4km.awsxclbin
                1ker_100k_8km.awsxclbin
                1ker_200k_4km.awsxclbin
                1ker_200k_8km.awsxclbin
                1ker_300k_4km.awsxclbin
                1ker_300k_8km.awsxclbin
                3ker_100k_4km.awsxclbin
                baseline_1ker_100k_4km.awsxclbin
                pipeline_1ker_100k_4km.awsxclbin
            host_executables/
                120_1ker_100k_4km
                1200_1ker_100k_4km
                1200_1ker_100k_8km
                1200_1ker_200k_4km
                1200_1ker_200k_8km
                1200_1ker_300k_4km
                1200_1ker_300k_8km
                1200_3ker_100k_4km
```

Remark: Once you are done with the creation of the FPGA images, delete all S3 buckets, except for the one you created, \$S3_EXE_BUCKET_NAME. For more information on how to delete S3 buckets, follow this [link](https://docs.aws.amazon.com/AmazonS3/latest/userguide/delete-bucket.html).

### 3.4.2 Execute on an AWS FPGA instance

1. Launch the Instance. Log into the appropriate AWS instance: f1.2xlarge, f1.4xlarge, or f1.16xlarge. To set up the instance, follow the instructions in documents/FPGArun.pdf.
2. Clone the GitHub repositories. Open the terminal. Then, clone our GitHub repository into a directory of your preference (e.g., /home/centos/):
    ```
    git clone https://github.com/AleP83/FPGA-Econ.git
    ```

3. Set the AWS credentials. Configure your AWS credentials by executing the following command in the terminal:
    ```
    aws configure
    ```
    Follow the steps here:

    ```
    $ aws configure
    AWS Access Key ID [*************xxxx]: <Your AWS Access Key ID>
    AWS Secret Access Key [**************xxxx]: <Your AWS Secret Access
        Key>
    Default region name: us-west-2
    Default output format: json
    ```

    For more information visit this [link](https://wellarchitectedlabs.com/common/documentation/aws_credentials/).


4. Modify the Makefile. Update settings in the code/Makefile as follows:

   - Set the AWS S3 Bucket Name: Specify the S3 bucket name by replacing S3-NAME-GOES-HERE
    ```
    S3_EXE_BUCKET_NAME := S3-NAME-GOES-HERE
    ```
    Remark: The S3 bucket name must be globally unique within AWS. If an error occurs during bucket creation, it may be due to the name being already in use by another user.

- Select the AWS region of the S3 bucket (default is us-west-2):
    ```
    AWS_REGION := us-west-2
    ```
    AWS Region and Bucket name should coincide with the ones used in the synthesis stage.

1. Modify Shell Script for FPGA Results. Update settings in the code/common/util/generate_fpga_results.sh as follows:
    - Set the AWS S3 Bucket Name: Specify the S3 bucket name by replacing S3-NAME-GOES-HERE
    ```
    S3_EXE_BUCKET_NAME="S3-NAME-GOES - HERE"
    ```
    Remark: The S3 bucket name must be globally unique within AWS. If an error occurs during bucket creation, it may be due to the name being already in use by another user.

    - Select the AWS region of the S3 bucket (default is us-west-2):
    ```
    AWS_REGION="us-west-2"
    ```
    AWS Region and Bucket name should coincide with the ones used in the synthesis stage.

6. Initiate tmux terminal session. To ensure your terminal session remains active throughout the execution, initiate a terminal multiplexer session:
```
tmux
```
The tmux command allows you to detach and reattach to terminal sessions without interruption. For example, to resume a tmux session with index 0 , use the following command:
```
tmux attach -t 0
```
For detailed instructions on how to use tmux, see this [guide](https://hamvocke.com/blog/a-quick-and-easy-guide-to-tmux/).

7. Execute application on F1 instances. Navigate to the code folder, and run the following commands to:
- Copy the executables from AWS S3 folder to the current AWS instance;
- Execute all the relevant exercises
- Transfer the generated results into the S3 folder.

In particular:
    (a) To replicate results on the f1.2xlarge instance execute on the tmux terminal:
    ```
    make fpga_results TABLE=all USE_AWS_S3_EXE=yes
    ```
    Estimated time: Approximately 20 hours.

    (b) To replicate results on the f1.4xlarge and f1.16xlarge instance execute on the tmux terminal of the respective instance:
    ```
    make fpga_results TABLE=3 USE_AWS_S3_EXE=yes
    ```
    Estimated time: Approximately 10 minutes for both.

Output. The command make fpga_results automatically saves the results in your S3 bucket, identified as \$S3_EXE_BUCKET_NAME, under the folder s3://\$S3_EXE_BUCKET_NAME/results/fpga/:

```
$S3_EXE_BUCKET_NAME/
    results/
    fpga/
        *.txt
        *.csv
        *.run_summary
        *.rpt
        *.xtxt
    *.log
```

Remark: Make sure to terminate your F1 instance! Even the smaller one (z1d.2xlarge) costs $1.65 \$ /$ hr.

### 3.5 Transfer the results to your local folder

The S3 bucket named \$S3_EXE_BUCKET_NAME contains the results of all CPU-C and FPGA-C model estimations. To download these results to your local machine, run the following file after making these changes:

- Launch the Instance. Log into an inexpensive AWS instance, say m5n.large.
- Download S3 bucket in AWS instance. Copy the S3 bucket into a directory of your choice within your AWS instance.

```
aws s3 cp --recursive s3://$S3_EXE_BUCKET_NAME/ ./s3-bucket/
```

- Compress the results. Compress the bucket results using tar tar -czvf s3-bucket-\$(date +\%Y-\%m-\%d).tar.gz s3-bucket/
- Copy the results in your local machine. Navigate into your local machine to the directory I_estimation_results/ and execute the following commands

```
instance_name="35-91-136-136"
key_directory="<Your AWS Access Key ID>"
region="<Your region>"
scp -i "${key_directory}" ec2-user@ec2-$instance_name.$region.compute.amazonaws.com:/home/ec2-user/s3-bucket-*.tar.gz ./
```


### 3.5.1 Clean AWS account

Once you are done with the AWS estimation, terminal all instances, delete all attached volumes and S3 buckets to avoid unintended charges.

## 4 Post Analysis

The post analysis phases uses the script ./II_post_analysis/main.m to process the inputs in ./I_estimation_results to replicate tables and figures in the paper. The results of the post analysis are organized as follows:

  - To organize the results for post analysis, navigate to the folder I_estimation_results, and execute the following commands to modify the file permissions of the script organize_files_for_post_analysis.sh and then execute it:
    ```
    chmod u+x organize_files_for_post_analysis.sh
    ./organize_files_for_post_analysis.sh
    ```
  - ./III_floats: ./II_post_analysis/main.m stores in the binary file ./III_floats/Tables.mat all tables referenced in the paper, which rely on data from the model estimation. Additionally, ./III_floats is structured with subfolders, each dedicated to storing individual tables, as indicated by their respective names.
  - ./IV_paper/results: The script ./II_post_analysis/main.m saves the results of the post-analysis in this folder, in the form of . txt files. These files are used by the $\mathrm{ET}_{\mathrm{E}} \mathrm{X}$ file to automatically populate numerical data of our paper.

    For computing the Euler Equation Errors in Table 7, ./II_post_analysis/main.m calls the function ./II_post_analysis/EEErrors/EEEfun.m. This function computes the euler equation errors for policy functions estimated on the FPGA, CPU-C and CPU-Matlab. To compute the Euler equation errors we use the code in Maliar et al. (2010) (Test.m) adapted to receive different grid sizes in ./II_post_analysis/EEErrors/EE_MMV.m.

## References

Cheela, B., A. DeHon, J. Fernández-Villaverde, and A. Peri (2023). A Beginner's Guide to Programming FPGAs for Economics: An Introduction to Electrical Engineering Economics. University of Pennsylvania.

Krusell, P. and A. A. Smith (1998). Income and wealth heterogeneity in the macroeconomy. Journal of Political Economy 106(5), 867-896.

Maliar, L., S. Maliar, and F. Valli (2010). Solving the incomplete markets model with aggregate uncertainty using the Krusell-Smith algorithm. Journal of Economic Dynamics and Control $34(1), 42-49$.

## A Compile CPU Executables

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 100 |
|  | \#define NKM_GRID 4 |
| /common/dev_options.h | \#define _LINEAR_SEARCH 0 |
|  | \#define _BINARY_SEARCH 0 |
|  | \#define _CUSTOM_BINARY_SEARCH 1 |
| In the terminal | make clean |
|  | make cpu_to_s3 CPU_EXE=1200_100k_4km |

Table 1: $N_{k}=100, N_{M}=4,1200$ Economies.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 200 |
|  | \#define NKM_GRID 4 |
| /common/dev_options.h. | \#define _LINEAR_SEARCH 0 |
|  | \#define _BINARY_SEARCH 0 |
|  | \#define _CUSTOM_BINARY_SEARCH 1 |
| In the terminal | make clean |
|  | make cpu_to_s3 CPU_EXE=1200_200k_4km |

Table 2: $N_{k}=200, N_{M}=4,1200$ Economies.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 300 |
|  | \#define NKM_GRID 4 |
| /common/dev_options.h | \#define _LINEAR_SEARCH 0 |
|  | \#define _BINARY_SEARCH 0 |
|  | \#define _CUSTOM_BINARY_SEARCH 1 |
| In the terminal | make clean |
|  | make cpu_to_s3 CPU_EXE=1200_300k_4km |

Table 3: $N_{k}=300, N_{M}=4,1200$ Economies.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 100 |
|  | \#define NKM_GRID 8 |
| /common/dev_options.h | \#define _LINEAR_SEARCH 0 |
|  | \#define _BINARY_SEARCH 0 |
|  | \#define _CUSTOM_BINARY_SEARCH 1 |
| In the terminal | make clean |
|  | make cpu_to_s3 CPU_EXE=1200_100k_8km |

Table 4: $N_{k}=100, N_{M}=8,1200$ Economies.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 200 |
|  | \#define NKM_GRID 8 |
| /common/dev_options.h | \#define _LINEAR_SEARCH 0 |
|  | \#define _BINARY_SEARCH 0 |
|  | \#define _CUSTOM_BINARY_SEARCH 1 |
| In the terminal | make clean |
|  | make cpu_to_s3 CPU_EXE=1200_200k_8km |

Table 5: $N_{k}=200, N_{M}=8,1200$ Economies.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 300 |
|  | \#define NKM_GRID 8 |
| /common/dev_options.h | \#define _LINEAR_SEARCH 0 |
|  | \#define _BINARY_SEARCH 0 |
|  | \#define _CUSTOM_BINARY_SEARCH 1 |
| In the terminal | make clean |
|  | make cpu_to_s3 CPU_EXE=1200_300k_8km |

Table 6: $N_{k}=300, N_{M}=8,1200$ Economies.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 100 |
|  | \#define NKM_GRID 4 |
| /common/dev_options.h | \#define _LINEAR_SEARCH 1 |
|  | \#define _BINARY_SEARCH 0 |
|  | \#define _CUSTOM_BINARY_SEARCH 0 |
| In the terminal | make clean |
|  | make cpu_to_s3 CPU_EXE=1200_linear |

Table 7: $N_{k}=100, N_{M}=4,1200$ Economies, Linear Search.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 100 |
|  | \#define NKM_GRID 4 |
| /common/dev_options.h | \#define _LINEAR_SEARCH 0 |
|  | \#define _BINARY_SEARCH 1 |
|  | \#define _CUSTOM_BINARY_SEARCH 0 |
| In the terminal | make clean |
|  | make cpu_to_s3 CPU_EXE=1200_binary |

Table 8: $N_{k}=100, N_{M}=4,1200$ Economies, Binary Search.

| File | Modify |
| :---: | :---: |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 100 <br> \#define NKM_GRID 4 |
| /common/dev_options.h | \#define _LINEAR_SEARCH 0 <br> \#define _BINARY_SEARCH 0 <br> \#define _CUSTOM_BINARY_SEARCH 1 |
| In the terminal | export PATH=\$PATH:\$HOME/openmpi/bin <br> export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:\$HOME/openmpi/lib <br> make clean <br> make openmpi_to_s3 OPENMPI_EXE=mpi_1200_100k_4km |

Table 9: OpenMPI: $N_{k}=100, N_{M}=4,1200$ Economies.

## B Create the FPGA images

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 100 |
|  | \#define NKM_GRID 4 |
|  | \#define _BASELINE 0 |
|  | \#define _PIPELINE 0 |
|  | \#define _WITHIN_ECONOMY 0 |
|  | \#define _ACROSS_ECONOMY 1 |
| /fpga/design.cfg | \#\#\# three kernel |
|  | [connectivity] |
|  | nk=runOnfpga:3:runOnfpga_1.runOnfpga_2.runOnfpga_3 |
| In the terminal | tmux |
|  | make clean |
|  | unset XCL_EMULATION_MODE |
|  | source \$AWS_FPGA_REPO_DIR/vitis_setup.sh |
|  | export PLATFORM_REPO_PATHS=\$(dirname \$AWS_PLATFORM) |
|  | export XCL_EMULATION_MODE=hw |
| make afi FPGA_BIN=3ker_100k_4km HOST_BIN=1200_3ker_100k_4km |  |

Table 10: Three-kernel design, $N_{k}=100, N_{M}=4,1200$ Economies.

| File | Modify |
| :---: | :---: |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 100 <br> \#define NKM_GRID 4 |
| /common/dev_options.h | \#define _BASELINE 0 <br> \#define _PIPELINE 0 <br> \#define _WITHIN_ECONOMY 1 <br> \#define _ACROSS_ECONOMY 0 |
| /fpga/design.cfg | $\# \# \#$ single kernel <br>  <br> nk=runOnfpga:1:runOnfpga_1 |
| In the terminal | tmux <br> make clean <br> unset XCL_EMULATION_MODE <br> source \$AWS_FPGA_REPO_DIR/vitis_setup.sh <br> export PLATFORM_REPO_PATHS=\$(dirname \$AWS_PLATFORM) <br> export XCL_EMULATION_MODE=hw <br> make afi FPGA_BIN=1ker_100k_4km HOST_BIN=1200_1ker_100k_4km |

Table 11: Single-kernel design, $N_{k}=100, N_{M}=4,1200$ Economies.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 200 |
|  | \#define NKM_GRID 4 |
|  | \#define _BASELINE 0 |
|  | \#define _PIPELINE 0 |
|  | \#define _WITHIN_ECONOMY 1 |
|  | \#define _ACROSS_ECONOMY 0 |
| /fpga/design.cfg | \#\#\# single kernel |
|  | [connectivity] |
|  | nk=runOnfpga:1:runOnfpga_1 |
| In the terminal | tmux |
|  | make clean |
|  | unset XCL_EMULATION_MODE |
|  | source \$AWS_FPGA_REPO_DIR/vitis_setup.sh |
|  | export PLATFORM_REPO_PATHS=\$(dirname \$AWS_PLATFORM) |
|  | export XCL_EMULATION_MODE=hw |
|  | make afi FPGA_BIN=1ker_200k_4km HOST_BIN=1200_1ker_200k_4km |

Table 12: Single-kernel design, $N_{k}=200, N_{M}=4,1200$ Economies.

| File | Modify |
| :---: | :---: |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 300 <br> \#define NKM_GRID 4 |
| /common/dev_options.h | \#define _BASELINE 0 <br> \#define _PIPELINE 0 <br> \#define _WITHIN_ECONOMY 1 <br> \#define _ACROSS_ECONOMY 0 |
| /fpga/design.cfg | $\# \# \#$ single kernel <br>  <br> nk=runOnfpga:1:runOnfpga_1 |
| In the terminal | tmux <br> make clean <br> unset XCL_EMULATION_MODE <br> source \$AWS_FPGA_REPO_DIR/vitis_setup.sh <br> export PLATFORM_REPO_PATHS=\$(dirname \$AWS_PLATFORM) <br> export XCL_EMULATION_MODE=hw <br> make afi FPGA_BIN=1ker_300k_4km HOST_BIN=1200_1ker_300k_4km |

Table 13: Single-kernel design, $N_{k}=300, N_{M}=4,1200$ Economies.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 100 |
|  | \#define NKM_GRID 8 |
| /common/dev_options.h. | \#define _BASELINE 0 |
|  | \#define _PIPELINE 0 |
|  | \#define _WITHIN_ECONOMY 1 |
|  | \#define _ACROSS_ECONOMY 0 |
| /fpga/design.cfg | \#\#\# single kernel |
|  | [connectivity] |
|  | nk=runOnfpga:1:runOnfpga_1 |
| In the terminal | tmux |
|  | make clean |
|  | unset XCL_EMULATION_MODE |
|  | source \$AWS_FPGA_REPO_DIR/vitis_setup.sh |
|  | export PLATFORM_REPO_PATHS=\$(dirname \$AWS_PLATFORM) |
|  | export XCL_EMULATION_MODE=hw |
|  | make afi FPGA_BIN=1ker_100k_8km HOST_BIN=1200_1ker_100k_8km |

Table 14: Single-kernel design, $N_{k}=100, N_{M}=8,1200$ Economies.

| File | Modify |
| :---: | :---: |
| //common/app.cpp | "\#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 200 <br> \#define NKM_GRID 8 |
| /common/dev_options.h | \#define _BASELINE 0 <br> \#define _PIPELINE 0 <br> \#define _WITHIN_ECONOMY 1 <br> \#define _ACROSS_ECONOMY 0 |
| /fpga/design.cfg | \#\#\# single kernel <br>  <br> nk=runOnfpga:1:runOnfpga_1 |
| In the terminal | tmux <br> make clean <br> unset XCL_EMULATION_MODE <br> source \$AWS_FPGA_REPO_DIR/vitis_setup.sh <br> export PLATFORM_REPO_PATHS=\$(dirname \$AWS_PLATFORM) <br> export XCL_EMULATION_MODE=hw <br> make afi FPGA_BIN=1ker_200k_8km HOST_BIN=1200_1ker_200k_8km |

Table 15: Single-kernel design, $N_{k}=200, N_{M}=8,1200$ Economies.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 1200 |
| /common/definitions.h | \#define NKGRID 300 |
|  | \#define NKM_GRID 8 |
| /common/dev_options.h | \#define _BASELINE 0 |
|  | \#define _PIPELINE 0 |
|  | \#define _WITHIN_ECONOMY 1 |
|  | \#define _ACROSS_ECONOMY 0 |
| /fpga/design.cfg | \#\#\# single kernel |
|  | [connectivity] |
|  | nk=runOnfpga:1:runOnfpga_1 |
| In the terminal | tmux |
|  | make clean |
|  | unset XCL_EMULATION_MODE |
|  | source \$AWS_FPGA_REPO_DIR/vitis_setup.sh |
| export PLATFORM_REPO_PATHS=\$(dirname \$AWS_PLATFORM) |  |
|  | export XCL_EMULATION_MODE=hw |
| make afi FPGA_BIN=1ker_300k_8km HOST_BIN=1200_1ker_300k_8km |  |

Table 16: Single-kernel design, $N_{k}=300, N_{M}=8,1200$ Economies.

| File | Modify |
| :--- | :--- |
| /common/app.cpp | \#define N_MODEL 120 |
| /common/definitions.h | \#define NKGRID 100 |
|  | \#define NKM_GRID 4 |
|  | \#define _BASELINE 1 |
|  | \#define _PIPELINE 0 |
|  | \#define _WITHIN_ECONOMY 0 |
|  | \#define _ACROSS_ECONOMY 0 |
| /fpga/design.cfg | \#\#\# single kernel |
|  | [connectivity] |
|  | nk=runOnfpga:1:runOnfpga_1 |
| In the terminal | tmux |
|  | make clean |
|  | unset XCL_EMULATION_MODE |
|  | source \$AWS_FPGA_REPO_DIR/vitis_setup.sh |
|  | export PLATFORM_REPO_PATHS=\$(dirname \$AWS_PLATFORM) |
| export XCL_EMULATION_MODE=hw |  |
| make afi FPGA_BIN=baseline_1ker_100k_4km HOST_BIN=120_1ker_100k_4km |  |

Table 17: Single-kernel Baseline design, $N_{k}=100, N_{M}=4,120$ Economies.

| File | Modify |
| :---: | :---: |
| /common/app.cpp | \# \#define N_MODEL 120 |
| /common/definitions.h | \#define NKGRID 100 <br> \#define NKM_GRID 4 |
| /common/dev_options.h | \#define _BASELINE 0 <br> \#define _PIPELINE 1 <br> \#define _WITHIN_ECONOMY 0 <br> \#define _ACROSS_ECONOMY 0 |
| //fpga/design.cfg | \#\#\# single kernel <br>  <br> nk=runOnfpga:1:runOnfpga_1 |
| In the terminal | tmux <br> make clean <br> unset XCL_EMULATION_MODE <br> source \$AWS_FPGA_REPO_DIR/vitis_setup.sh <br> export PLATFORM_REPO_PATHS=\$(dirname \$AWS_PLATFORM) <br> export XCL_EMULATION_MODE=hw <br> make afi FPGA_BIN=pipeline_1ker_100k_4km HOST_BIN=120_1ker_100k_4km |

Table 18: Single-kernel only-pipelining design, $N_{k}=100, N_{M}=4,120$ Economies.

