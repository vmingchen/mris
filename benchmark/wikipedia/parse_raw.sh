#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./parse_raw.sh -h
# 
#   DESCRIPTION:  Parse raw data from Wikipedia pagecount. Input is read from
#   stdin. Output will be appended into output1 and output2.
#
#	Output format: 
#	#### filename	#errorline	#image_requests/#total_requests #image_lines/#total_lines
#   size_in_sector1	frequency1
#   size_in_sector2	frequency1
#   ......
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

if [ $# -ne 3 ]; then
	echo "usage: $0 name output1 output2"
	exit 1;
fi

awk -v name="$1" -v out1="$2" -v out2="$3" '
	{
		if (NF != 4) {
			nerror += 1;
		} else {
			nlines += 1;
			nsector = int(($4 + 511)/512);
			nrequest += $3;
			requests[nsector] += $3;
			if (tolower($2) ~ /.(jpg|png|gif)/){
				nimagelines += 1;
				nimage += $3;
				images[nsector] += $3;
			}
		}
	}
	END {
		printf("####### %s %d %d/%d %d/%d #####\n", name, nerror,
				nimage, nrequest, imagelines, nlines) >> out1;
		for (n in requests) {
			printf("%d\t%d\n", n, requests[n]) >> out1;
		}
		printf("####### %s done #####\n", name) >> out1;

		printf("####### %s %d %d/%d %d/%d #####\n", name, nerror,
				nimagelines, nlines, nimage, nrequest) >> out2;
		for (n in images) {
			printf("%d\t%d\n", n, images[n]) >> out2;
		}
		printf("####### %s done #####\n", name) >> out2;
	}' -
