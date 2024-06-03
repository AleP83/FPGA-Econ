#!/bin/bash

# First step is to list all the afi associated with a user id and copy to a file (list_all_afi.json)
# aws ec2 describe-fpga-images --filters Name=owner-id,Values=801285156158 > list_all_afi.json

# Parse the JSON log and extract all FpgaImageIds
FpgaImageIds=($(jq -r '.FpgaImages[].FpgaImageId' list_all_afi.json))

# Loop through each FpgaImageId
for FpgaImageId in "${FpgaImageIds[@]}"; do
    # Check if FpgaImageId is not empty
    if [ -n "$FpgaImageId" ]; then
        # Run the AWS CLI command with the extracted FpgaImageId
        aws ec2 --region us-east-1 modify-fpga-image-attribute --fpga-image-id $FpgaImageId --operation-type add --user-groups all
    else
        echo "Error: Unable to extract FpgaImageId from the log."
    fi
done
