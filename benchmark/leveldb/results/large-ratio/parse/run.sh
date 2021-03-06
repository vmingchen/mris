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

workload=mris_ratio

function parse() {
	echo -ne "#setup\tratio\tepoch\top/s\tMB/s\n"
	for setup in ssd hybrid sata; do
		for ratio in 1 2 4 8 16 32 64; do
			for epoch in 1 2 3; do
				echo -ne "$setup\t$ratio\t$epoch\t"
				local filename=../${workload}_${setup}.${ratio}.${epoch}.log 
				grep -w "mris_facebook" $filename | 
					awk '{printf("%s\t%s\n", 1000000/$3, $5);}'
			done
		done
	done
}

# get average of columns in iostat csv file obtained by iostat2csv.sh
function get_iostat_col_avg() {
	local n=$1
	local filename=$2
	# ignore the header and the first record
	# (sum * 0.5)/1024 turns sec/s to mb/s
	awk -v n="$n" -v FS=',' '(NR > 2) {
		sum += $n;
	} END {
		printf("%.2f", sum / (NR - 2));
	}' ${filename}
}


# get average thput from iostat csv file obtained by iostat2csv.sh
function get_iostat_thput() {
	local filename=$1
	# ignore the header and the first record
	# (sum * 0.5)/1024 turns sec/s to mb/s
	awk -v FS=',' '(NR > 2) {
		sum += $5;
	} END {
		printf("%.2f", (sum * 0.5) / (1024 * (NR - 2)));
	}' ${filename}
}

function parse_iostat() {
	echo -ne "#setup\tratio\tepoch\tMB/s(SSD)\tMB/s(SATA)\tr/s(SSD)\tr/s(SATA)\trqsz(SSD)\trqst(SATA)\n"
	for setup in ssd hybrid sata; do
		for ratio in 1 2 4 8 16 32 64; do
			for epoch in 1 2 3; do
				local filename=../${workload}_${setup}.${ratio}.${epoch}.iostat
				iostat2csv.sh $filename
				# get thput of the SSD and SATA respectively
				ssd_thput=`get_iostat_thput $filename.sdb1.csv`
				sata_thput=`get_iostat_thput $filename.sdc1.csv`
				ssd_ops=`get_iostat_col_avg 3 $filename.sdb1.csv`
				sata_ops=`get_iostat_col_avg 3 $filename.sdc1.csv`
				ssd_rqsz=`get_iostat_col_avg 7 $filename.sdb1.csv`
				sata_rqsz=`get_iostat_col_avg 7 $filename.sdc1.csv`
				echo -e "${setup}\t${ratio}\t${epoch}\t${ssd_thput}\t${sata_thput}\t${ssd_ops}\t${sata_ops}\t${ssd_rqsz}\t${sata_rqsz}"
			done
		done
	done
}

function parse_column() {
	local n="$1"
	local filename="$2"
	for ratio in 1 2 4 8 16 32 64; do
		echo -ne "$ratio\t"
		for setup in ssd hybrid sata; do
			echo -ne "$setup\t"
			for i in $n; do
				grep "$setup	$ratio	" $filename | 
					cut -f $i | 
						calc.py -s | 
							tr '\n' '\t'
			done
		done
		echo ""
	done
}

function parse_ops() {
	echo -e "#ratio\tsetup\tops\tstddev" > mris_ratio_ops.dat
	parse_column 4 mris_ratio.dat >> mris_ratio_ops.dat
}

function parse_thput() {
	echo -e "#ratio\tsetup\tmb/s\tstddev" > mris_ratio_mb.dat
	parse_column 5 mris_ratio.dat >> mris_ratio_mb.dat
}

function parse_iostat_thput() {
	echo -e "#ratio\tsetup\tssd\tstddev\tsata\tstddev" 
	parse_column "4 5" mris_ratio_iostat.dat 
}

function parse_iostat_ops() {
	echo -e "#ratio\tsetup\tssd\tstddev\tsata\tstddev" 
	parse_column "6 7" mris_ratio_iostat.dat 
}

function parse_iostat_rqsz() {
	echo -e "#ratio\tsetup\tssd\tstddev\tsata\tstddev" 
	parse_column "6 7" mris_ratio_iostat.dat 
}

#parse > mris_ratio.dat
#parse_ops
#parse_thput

parse_iostat > mris_ratio_iostat.dat
parse_iostat_thput > mris_ratio_iostat_thput.dat
parse_iostat_ops > mris_ratio_iostat_ops.dat
parse_iostat_rqsz > mris_ratio_iostat_rqsz.dat
