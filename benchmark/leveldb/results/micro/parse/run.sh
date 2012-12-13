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
export PATH="/bin:/usr/bin:/sbin"             
IFS=$' \t\n'                            # reset IFS
unset -f unalias                        # make sure unalias is not a function
\unalias -a                             # unset all aliases
ulimit -H -c 0 --                       # disable core dump
hash -r                                 # clear the command path hash

#function parse() {
	#local workload=$1
	#local setup=$2
	#epoch=`wc -l ../${workload}_${setup}.*.log`
	#grep -w "${workload}" ../${workload}_${setup}.*.log | 
		#awk '{printf("%.2f\t%.2f\n", 1000000/$3, $5);}'
#}

echo "#workload setup epoch op/s MB/s"
for workload in mris_ran_wt mris_seq_wt; do
	for setup in ssd sata hybrid; do
		for epoch in 1 2 3; do
			echo -ne "$workload\t$setup\t$epoch\t"
			filename=../${workload}_${setup}.${epoch}.log 
			cat $filename | tr '\r' '\n' | 
				awk -v wl="$workload" '($1 == wl) {printf("%s\t%s\n", $3, $5);}'
		done
	done
done
