#!/bin/sh

# Shebang line indicates the interpreter to be used (in this case,shell)

# Assign provided args to variables
filesdir=$1
searchstr=$2

# Check if the scipt is provided with two args, if not print error message and return with error code 1
if [ $# -ne 2 ]; then
    echo "Error:1 The Invalid number of arguments "
    exit 1
fi

# Check if filesdir is a valid directory
if [ -d "$filesdir" ]; then

	#Calculate total files present in the file system for present directory
	Total_files=$(find $filesdir -type f | wc -l)

	#To store matching lines
	Lines_matching=0

	#To check how many matching lines are present.
	for i in $(find $filesdir -type f)
		do
    			if grep -q "$searchstr" "$i"; then
        			Lines_matching=$((Lines_matching+1))
    			fi
	done

echo "The number of files are $Total_files and the number of matching lines are $Lines_matching"
else
    echo "Error: 1 The provided directory path is not valid"
    exit 1
fi

