#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./run.sh 
# 
#   DESCRIPTION:  parse ratio results and prepare for plotting
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
	echo -ne "#setup\tratio\tepoch\top/s\tMB/s\n"
	workload=mris_ratio
	for setup in ssd hybrid sata; do
		for ratio in 1 2 4 8 16 32 64; do
			for epoch in 1 2 3; do
				echo -ne "$setup\t$ratio\t$epoch\t"
				filename=../${workload}_${setup}.${ratio}.${epoch}.log 
				grep -w "mris_facebook" $filename | 
					awk '{printf("%s\t%s\n", 1000000/$3, $5);}'
			done
		done
	done
}

function parse_column() {
	n=$1
	for ratio in 1 2 4 8 16 32 64; do
		echo -ne "$ratio\t"
		for setup in ssd hybrid sata; do
			echo -ne "$setup\t"
			grep "$setup	$ratio	" mris_ratio.dat | 
				cut -f $n | 
					calc.py -s | 
						tr '\n' '\t'
		done
		echo ""
	done
}

function parse_ops() {
	echo -e "#setup\tops\tstddev" > mris_ratio_ops.dat
	parse_column 4 >> mris_ratio_ops.dat
}

function parse_thput() {
	echo -e "#setup\tmb/s\tstddev" > mris_ratio_thput.dat
	parse_column 5 >> mris_ratio_thput.dat
}

parse > mris_ratio.dat
parse_ops
parse_thput
