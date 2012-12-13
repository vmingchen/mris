#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./run.sh 
# 
#   DESCRIPTION:  parse micro results and prepare for plotting
# 
#        AUTHOR: Ming Chen, mchen@cs.stonybrook.edu
#
#===============================================================================

set -o nounset                          # treat unset variables as an error
set -o errexit                          # stop script if command fail
export PATH="/bin:/usr/bin:/sbin:../../../../../software"             
IFS=$' \t\n'                            # reset IFS
unset -f unalias                        # make sure unalias is not a function
\unalias -a                             # unset all aliases
ulimit -H -c 0 --                       # disable core dump
hash -r                                 # clear the command path hash

function parse() {
	echo "#workload setup epoch op/s MB/s"
	for workload in mris_ran_wt mris_seq_wt; do
		for setup in ssd sata hybrid; do
			for epoch in 1 2 3; do
				echo -ne "$workload\t$setup\t$epoch\t"
				filename=../${workload}_${setup}.${epoch}.log 
				cat $filename | tr '\r' '\n' | 
					awk -v wl="$workload" '($1 == wl) {
						printf("%s\t%s\n", 1000000.0/$3, $5);
					}'
			done
		done
	done
}

#parse > micro.dat

function parse_column() {
	local n="$1"
	local filename="$2"
	for workload in mris_ran_wt mris_seq_wt; do
		echo -ne "${workload:5:3}\t"
		for setup in ssd hybrid sata; do
			echo -ne "$setup\t"
			for i in $n; do
				grep "^$workload	$setup	" $filename | 
					cut -f $i | 
						calc.py -s | 
							tr '\n' '\t'
			done
		done
		echo ""
	done
}

parse_column 4 micro.dat > micro_ops.dat
parse_column 5 micro.dat > micro_thput.dat
