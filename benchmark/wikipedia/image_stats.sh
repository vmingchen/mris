#!/bin/bash - 
#=============================================================================
#
#         USAGE:  ./image_stats.sh 
# 
#   DESCRIPTION:  Get image statistics from Wikipedia. The results are saved in
#   data/. The downloaded files are saved on dolphin.
#		- Distribution of image object size in sectors
#		- Distribution of object size in sectors
#		- Number of total request
#		- Number of image requests
# 
#        AUTHOR: Ming Chen, mchen@cs.stonybrook.edu
#
#=============================================================================

set -o nounset                          # treat unset variables as an error
set -o errexit                          # stop script if command fail
#export PATH="/bin:/usr/bin:/sbin"             
IFS=$' \t\n'                            # reset IFS
unset -f unalias                        # make sure unalias is not a function
\unalias -a                             # unset all aliases
ulimit -H -c 0 --                       # disable core dump
hash -r                                 # clear the command path hash

url_header="http://dumps.wikimedia.org/other/pagecounts-raw/2012/2012-01/"
data_dir="/home/mchen/Downloads/wikipedia"

function download_file() {
	local name="$1"
	local md5="$2"
	local trial=""

	for trial in `seq 5`; do
		if wget "$url_header/$name"; then
			local md52=`md5sum $name | awk '{print $1;}'`
			if [ "x$md52" = "x$md5" ]; then
				mv $name "$data_dir/$name"
				return 0
			fi
		else
			# remove unsuccessful file if exists
			[ -f $name ] && rm -f $name
			echo "========== trial $trial failed; sleep and retry later"
			sleep $((60*$trial))
		fi
	done
	exit 1;
}

# load MD5 and file names into array
nfiles=`cat md5week1.txt | wc -l`
i=1
while read md5 fname; do
	md5s[$i]=$md5
	names[$i]=$fname
	i=$((i+1))
done < md5week1.txt

for i in `seq $nfiles`; do
	md5=${md5s[$i]}
	name=${names[$i]}
	nmlen=${#name}
	id=${name:$nmlen-18:15}

	echo "========== processing $name =========="

	# only process pagecount file if it is not processed before
	if grep -s -q "done" "image-$id.out"; then
		continue
	fi

	# check if we need to download file
	download=y
	if [ -f "$data_dir/$name" ]; then
		# check MD5
		md52=`md5sum $data_dir/$name | awk '{print $1;}'`
		if [ "x$md52" = "x$md5" ]; then
			download=n
			echo "========== $name already exists =========="
		fi
	fi
	
	if [ "x$download" = "xy" ]; then
		echo "========== downloading $name =========="
		download_file $name $md5
		echo "========== download done =========="
	fi


	echo "========== parsing $name =========="
	zcat "$data_dir/$name" | ./parse_raw.sh "$name" \
		"request-$id.out" "image-$id.out"
	echo "========== $name DONE =========="
done
