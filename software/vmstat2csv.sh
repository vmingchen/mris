#!/bin/bash
#
# Copyright (c) 2011 Jack Ma
# Copyright (c) 2011 Vasily Tarasov
# Copyright (c) 2011 Santhosh Kumar Koundinya
# Copyright (c) 2011 Erez Zadok
# Copyright (c) 2011 Geoff Kuenning
# Copyright (c) 2011 Stony Brook University
# Copyright (c) 2011 The Research Foundation of SUNY
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# Name: vmstat2csv.sh
#
# DESCRIPTION:
# A script that parses text output from vmstat and generates a CSV file.
# The first row will contain the field names. The output file name will be of
# the form [<prefix>.]<input_file_name>[.<suffix>].csv
#
# USAGE:
# vmstat2csv.sh <input file name> [<output file name prefix>]
#   [<output file name suffix>]
#
# Return: 0 on success, non zero on failure.
#
# ASSUMPTIONS:
# The input file is expected to contain many 'records' of the following format:
#
###############################################################################
# procs -----------memory---------- ---swap-- -----io---- --system-- -----cpu-----
# r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
# 2  0      0 3843784   7564  21400    0    0     9     7    1   11  0  0 100  0  0	
# 0  1      0 3801536    644  50560    0    0   662  3450 1231  496  1  2 54 42  0	
# 0  1      0 3793104    652  58760    0    0   719     2 1492  576  0  1 50 48  0	
# 0  1      0 3792732    940  58760    0    0   753     2 1506  584  0  1 50 49  0	
# 0  1      0 3792608    956  58768    0    0   716   854 1422  574  0  2 50 49  0	
###############################################################################
#
# Use of the following GNU tools are made: grep, tr, cut. These tools must be
# available on the path.

# Helper function that prints usage.
show_usage()
{
	echo $0 "<input file name> [<output file name prefix>]"\
		"[<output file name suffix>]"
}

format_output_filename()
{
	OUTPUTFILE=""

	if [ ! -z "$PREFIX" ]; then
		OUTPUTFILE=${PREFIX}
	fi

	OUTPUTFILE=${OUTPUTFILE}$INPUTFILE

	if [ ! -z "$SUFFIX" ]; then
		OUTPUTFILE=${OUTPUTFILE}${SUFFIX}
	else
		OUTPUTFILE=${OUTPUTFILE}".csv"
	fi
}

extract_vm_stats()
{
	OUTPUTFILE=$1 # Output file name is the first argument.

	# A vmstat file is already well formatted. Just skip the first line and
	# use a comma as a delimiter.
	echo -n \# > $OUTPUTFILE
	cat $INPUTFILE \
	| (read; cat) \
	| tr -s ' ' \
	| cut - -d ' ' -f 2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 \
		--output-delimiter ',' \
	>> $OUTPUTFILE
}

main()
{
	case $# in
	3) SUFFIX=$3 ;& # Fall through.
	2) PREFIX=$2 ;& # Fall through.
	1) INPUTFILE=$1 ;;
	*)
		echo "Incorrect number of arguments. See usage below."
		show_usage
		exit 1
	esac

	# Does the input file exist and is a regular file?
	if [ ! -f $INPUTFILE ]; then
	        echo "Input file '"${INPUTFILE}"' does not exist."
	        exit 1
	fi

	# Does the input file contain data?
	if [ ! -s $INPUTFILE ]; then
		echo "Input file '"${INPUTFILE}"' is of zero size."
		exit 1
	fi

	# Extract VM stats.
	format_output_filename
	extract_vm_stats $OUTPUTFILE

	# All done.
	return 0
}

INPUTFILE=""
OUTPUTFILE=""
PREFIX=""
SUFFIX=""

# Start processing.
main $@

