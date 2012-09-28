#!/bin/env python
import sys
import math

def hsize(size):
	scales = "BKMGTP"
	i = 0
	while size >= 1024:
		size = size >> 10
		i += 1
	return "%d%s" % (size, scales[i])

if __name__=="__main__":
	if len(sys.argv) != 2:
		print "usage: %s infile" % sys.argv[0]
		sys.exit(1)

	hist = {}
	#for line in sys.stdin:
	for line in open(sys.argv[1]):
		(site, name, freq, size) = line.split()
		if float(size) <= 0:
			continue
		sz_in_sector = math.ceil(float(size) / 512.0)
		level = int(math.log(sz_in_sector, 2))
		if level in hist: 
			hist[level] += int(freq);
		else:
			hist[level] = int(freq)

	units = {}
	base = 512
	for i in range(1 + max(hist.keys())):
		units[i] = hsize(base)
		base = base << 1

	for (i, v) in hist.items():
		print "%s\t%d\t%d" % (units[i], i, v)
