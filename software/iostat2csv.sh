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
# Name: iostat2csv.sh
#
# DESCRIPTION:
# A script that parses text output from iostat and generates two CSV files:
# One for CPU statistics and the other for disk statistics. The file names
# generated for the disk statistics and cpu statistics files are 
# [<prefix>.]<input_file_name><device name>[.<suffix>].csv
# and [<prefix>.]<input_file_name>.cpu[.<suffix>].csv
# respectively. Specifying a prefix and/or a suffix is optional. The first
# row in both output file will contain a header with field names.
#
# USAGE:
# iostat2csv.sh <input file name> [device name] [<output file name prefix>]
#   [<output file name suffix>]
#
# Return: 0 on success, non zero on failure.
#
# ASSUMPTIONS:
# The input file is expected to contain many 'records' of the following format:
#
###############################################################################
# 09/09/2011 01:34:57 PM
# avg-cpu:  %user   %nice %system %iowait  %steal   %idle
#           0.01    0.00    0.32    0.08    0.00   99.58
#
# Device:         rrqm/s   wrqm/s     r/s     w/s   rsec/s   wsec/s avgrq-sz avgqu-sz   await  svctm  %util
# sda               0.09     0.61    0.07    0.11     6.37     5.78    66.35     0.00   24.40   5.71   0.10
# sdb               0.05    20.37    3.68    0.18    29.60    22.01    13.37     0.07   16.93   0.19   0.08
###############################################################################
#
# Use of the following GNU tools are made: grep, tr, cut. These tools must be
# available on the path.

# Helper function that prints usage.
show_usage()
{
	echo $0 "<input file name> [device name] [<output file name prefix>]"\
		"[<output file name suffix>]"
}

format_output_filename()
{
	TYPE=$1 # Should typically be 'cpu' or the device name.
	OUTPUTFILE=""

	if [ ! -z "$PREFIX" ]; then
		OUTPUTFILE=${PREFIX}
	fi

	OUTPUTFILE=${OUTPUTFILE}${INPUTFILE}.${TYPE}

	if [ ! -z "$SUFFIX" ]; then
		OUTPUTFILE=${OUTPUTFILE}${SUFFIX}
	else
		OUTPUTFILE=${OUTPUTFILE}".csv"
	fi
}

extract_cpu_stats()
{
	OUTPUTFILE=$1 # Output file name is the first argument.

	# Extract and print the header.
	# Input: 'avg-cpu:  %user   %nice %system %iowait  %steal   %idle'
	# Output: '%user,%nice,%system,%iowait,%steal,%idle'
	echo -n \# > $OUTPUTFILE
	grep -m 1 'avg-cpu' $INPUTFILE | tr -s ' ' | \
		cut - -d ' ' -f 2,3,4,5,6,7 --output-delimiter ',' >> $OUTPUTFILE

	# Now extract the body.
	# Input: '          0.01    0.00    0.32    0.08    0.00   99.58'
	# Output: '0.01,0.00,0.32,0.08,0.00,99.58'
	grep -A 1 'avg-cpu' $INPUTFILE \
		| grep -v 'avg-cpu' - \
		| grep -v '-' - \
		| tr -s ' ' \
		| cut - -d ' ' -f 2,3,4,5,6,7 --output-delimiter ',' \
		>> $OUTPUTFILE
}

extract_disk_stats()
{
	OUTPUTFILE=$1
	# DEVICENAME must be initialized appropriately.

	# Extract and print the header.
	# Input: 'Device:         rrqm/s   wrqm/s     r/s     w/s   rsec/s   wsec/s avgrq-sz avgqu-sz   await  svctm  %util'
	# Output: 'rrqm/s,wrqm/s,r/s,w/s,rsec/s,wsec/s,avgrq-sz,avgqu-sz,await,svctm,%util'
	echo -n \# > $OUTPUTFILE
	grep -m 1 'Device:' $INPUTFILE | tr -s ' ' | \
		cut - -d ' ' -f 2,3,4,5,6,7,8,9,10,11,12 --output-delimiter ',' >> $OUTPUTFILE

	# Now extract the body.
	# Input: 'sda               0.09     0.61    0.07    0.11     6.37     5.78    66.35     0.00   24.40   5.71   0.10'
	# Output: '0.09,0.61,0.07,0.11,6.37,5.78,66.35,0.00,24.40,5.71,0.10'
	grep $DEVICENAME $INPUTFILE | tr -s ' ' | \
		cut - -d ' ' -f 2,3,4,5,6,7,8,9,10,11,12 --output-delimiter ',' >> $OUTPUTFILE
}

main()
{
	case $# in
	4) SUFFIX=$4 ;& # Fall through.
	3) PREFIX=$3 ;& # Fall through.
	2) DEVICENAME=$2;& # Fall through.
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

	# Extract CPU stats.
	format_output_filename "cpu"
	extract_cpu_stats $OUTPUTFILE

	# Getting the list of drives
	if [ -z "$DEVICENAME" ]; then
		LIST_OF_DRIVES=`awk 'BEGIN {LIST_OBTAINED = 0}
		 /Device:/,/^$/ {if (!LIST_OBTAINED) print $1; inblock = 1}
		/^$/ && inblock == 1 { LIST_OBTAINED = 1}' $INPUTFILE`
		LIST_OF_DRIVES=`echo $LIST_OF_DRIVES | cut -d ' ' -f 2-`
	else
		LIST_OF_DRIVES="$DEVICENAME"
	fi

	# Extract disk stats.
	for DEVICENAME in $LIST_OF_DRIVES
	do
		format_output_filename $DEVICENAME
		extract_disk_stats $OUTPUTFILE
	done

	# All done.
	return 0
}

INPUTFILE=""
OUTPUTFILE=""
DEVICENAME=""
PREFIX=""
SUFFIX=""

# Start processing.
main $@

