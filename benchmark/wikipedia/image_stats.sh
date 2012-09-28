#!/bin/bash - 
#=============================================================================
#
#         USAGE:  ./image_stats.sh 
# 
#   DESCRIPTION:  Get image statistics from Wikipedia.
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
export PATH="/bin:/usr/bin:/sbin"             
IFS=$' \t\n'                            # reset IFS
unset -f unalias                        # make sure unalias is not a function
\unalias -a                             # unset all aliases
ulimit -H -c 0 --                       # disable core dump
hash -r                                 # clear the command path hash

url_header="http://dumps.wikimedia.org/other/pagecounts-raw/2012/2012-01/"
data_dir="/home/mchen/Downloads/wikimedia"
request_out="request.out"
image_out="image.out"
for d in `seq 1 7`; do
	for n in `seq 0 23`; do
		name=`printf "pagecounts-201201%02d-%02d0000.gz" $d $n`
		wget "$url_header/$name" "$data_dir/$name"
		zcat "$data_dir/$name" | ./parse_raw.sh "$name" "$request_out" "$image_out"
	done
done
