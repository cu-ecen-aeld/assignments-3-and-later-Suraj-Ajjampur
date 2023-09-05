#!/bin/sh

# Shebang line indicates the shell interpreter to be used

# Check if the required arguments are provided
if [ $# -ne 2 ]; then
    echo "Error: 1 - Invalid number of Arguements provided"
    exit 1
fi

# Assign provided args to variables
writefile=$1
writestr=$2

# Get subpath from the given arguments
path=$(dirname ${writefile})

# Create all dictories [-p arg tells the terminal to create a parent directory if not present]
mkdir -p "$path"

# Write the string content onto the file using redirection
echo "$writestr" > "$writefile"
