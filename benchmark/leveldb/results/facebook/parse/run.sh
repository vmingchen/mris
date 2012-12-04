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
	echo -ne "#setup\tepoch\top/s\tMB/s\n"
	for workload in mris_facebook; do
		for setup in ssd hybrid sata; do
			for epoch in 1 2 3; do
				echo -ne "$setup\t$epoch\t"
				filename=../${workload}_${setup}.17.${epoch}.log 
				grep -w "${workload}" $filename | 
					awk '{printf("%s\t%s\n", 1000000/$3, $5);}'
			done
		done
	done
}

function parse_column() {
	n=$1
	for setup in ssd hybrid sata; do
		echo -ne "$setup\t"
		grep "^$setup" mris_facebook.dat | 
			cut -f $n | 
				calc.py -s
	done
}

function parse_ops() {
	echo -e "#setup\tops\tstddev" > mris_facebook_ops.dat
	parse_column 3 >> mris_facebook_ops.dat
}

function parse_thput() {
	echo -e "#setup\tmb/s\tstddev" > mris_facebook_thput.dat
	parse_column 4 >> mris_facebook_thput.dat
}

parse > mris_facebook.dat

parse_ops

parse_thput
