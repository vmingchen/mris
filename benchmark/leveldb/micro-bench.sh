#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./micro-bench.sh 
# 
#   DESCRIPTION:  micro-benchmark of MRIS
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

function run_bench() {
	benchmark="$1"
	./db_bench --histogram=1 --num=1000 --benchmarks=$benchmark \
		--value_size=-1 --compression_ratio=1.0 --threads=1	\
		--db=${benchmark}_db >${benchmark}.log 2>&1
}

DIR=/mnt/ssd
cp db_bench $DIR
cd $DIR

#run_bench mris_ran_wt
run_bench mris_seq_wt
