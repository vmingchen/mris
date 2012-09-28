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

awk '
	function m2hsize(msize, istyle,		styles, units, base, tmp, i)
	{
		styles[0] = ":K:M:G:T";
		styles[1] = "B:KB:MB:GB:TB";
		split(styles[istyle], units, ":");
		base = 1;
		tmp = msize;
		# index starts from 1
		for (i = 1; tmp >= 1024; ++i) {
			tmp = rshift(tmp, 10);
			base = lshift(base, 10);
		}
		print units[2];
		return sprintf("%d%s", int(msize/base), units[i+1]);
	}
	{
		sz_in_sector = int($4/512)
		base = 512;
		for (i = 0; sz_in_sector > 0; ++i) {
			sz_in_sector = rshift(sz_in_sector, 1);
			base = lshift(base, 1);
		}
		hist[i] += $3;
		printf("%d ", base);
		printf("%d\t%s\n", base, m2hsize(base));
	} 
	END {
		for (sz in hist) {
			print units[sz], sz, hist[sz];
		}
	}' - | sort -k1,1n 
