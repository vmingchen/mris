#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./facebook-bench.sh 
# 
#   DESCRIPTION:  facebook-benchmark of MRIS
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

# number of MRI for test
NUM=100000
NREAD=1000

LEVELDB_HOME=/home/mchen/build/mris/benchmark/leveldb

RES=$LEVELDB_HOME/results
DIR=/mnt/ssd

benchmark=mris_facebook

function remount() {
	if mount -l | grep -q '^/dev/sdb1 '; then
		while ! umount /dev/sdb1; do
			sleep 1
		done
	fi
	[ -b /dev/sdb1 ] && mount /dev/sdb1 /mnt/ssd
	if mount -l | grep -q '^/dev/sdc1 '; then
		while ! umount /dev/sdc1; do
			sleep 1
		done
	fi
	[ -b /dev/sdc1 ] && mount /dev/sdc1 /mnt/sata
}

function use_ssd() {
	rm -f /mnt/largespace
	ln -s /mnt/ssd /mnt/largespace
}

function use_sata() {
	rm -f /mnt/largespace
	ln -s /mnt/sata /mnt/largespace
}

function setup() {
	local name=$1
	if [ "x$name" = "xssd" ]; then
		use_ssd && DIR=/mnt/ssd
	elif [ "x$name" = "xsata" ]; then
		use_sata && DIR=/mnt/sata
	elif [ "x$name" = "xhybrid" ]; then
		use_sata && DIR=/mnt/ssd
	else
		echo "unknown setup $name" > /dev/stderr
		exit 1;
	fi
}

function clean() {
	local dbname="$1"
	rm -rf /mnt/largespace/${dbname}
	rm -rf $DIR/${dbname}
}

function populate_db() {
	local setup_name="$1"
	local dbname="${benchmark}_${setup_name}_db"

	# ignore if db already exists
	if [ -d /mnt/largespace/${dbname} -a -d $DIR/${dbname} ]; then
		echo "--- ${dbname} already exists"
		return
	fi

	./db_bench --histogram=1 --num=$NUM --benchmarks=mris_seq_wt \
		--value_size=-1 --compression_ratio=1.0 --threads=1	\
		--db=${DIR}/${dbname}
}

function run_bench() {
	local ratio="$1"
	local setup_name="$2"
	local epoch="$3"
	local result=${RES}/${benchmark}_${setup_name}.${ratio}.$epoch
	local dbname="${benchmark}_${setup_name}_db"

	# clear cache
	remount

	vmstat -n 5 >${result}.vmstat &
	PID_VMSTAT=$!

	iostat -t -x /dev/sdb1 /dev/sdc1 5 >${result}.iostat &
	PID_IOSTAT=$!

	./db_bench --histogram=1 --num=$NUM --benchmarks=$benchmark \
		--value_size=-1 --compression_ratio=1.0 --threads=1	\
		--mris_ratio=$ratio --reads=$NREAD --use_existing_db=1 \
		--db=${DIR}/${dbname} >${result}.log 2>&1

	kill $PID_IOSTAT $PID_VMSTAT
}

for setup_name in ssd sata hybrid; do
	setup $setup_name
	echo "--- [begin] setup: $setup_name; benchmark: $benchmark"
	echo "--- populating db"
	populate_db $setup_name
	echo "--- db populated"
	for ratio in 17; do
	#for ratio in 1 2 4 8 16 32 64; do
		for epoch in 1 2 3; do
			echo "--- epoch: $epoch"
			run_bench $ratio $setup_name $epoch
		done
		echo "--- [end] setup: $setup_name; benchmark: $benchmark"
	done
done
