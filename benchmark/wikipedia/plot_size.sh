#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./plot_size.sh 
# 
#   DESCRIPTION:  plot distribution of I/O size of Wikipedia requests
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

awk '{
		sz_in_sector = int($4/512)
		for (i = 0; sz_in_sector > 0; ++i) 
			sz_in_sector = rshift(sz_in_sector, 1);
		hist[i] += $3;
	} END {
		for (sz in hist) {
			print sz, hist[sz];
		}
	}' -
