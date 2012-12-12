#!/usr/bin/python
'''
Calculate statistics of file sizes (logarithmic).
'''
import sys
import math
import get_hist

if __name__=="__main__":
	if len(sys.argv) != 2:
		print "usage: %s infile" % sys.argv[0]
		sys.exit(1)

	hist = {}
	for line in open(sys.argv[1]):
		if line[0] == "#":
			continue
		(size, freq) = line.split()
		size = float(size)
		if size <= 0:
			continue
		level = int(math.log(size, 2))
		if level in hist: 
			hist[level] += int(freq);
		else:
			hist[level] = int(freq)

	units = {}
	base = 512
	for i in range(1 + max(hist.keys())):
		units[i] = get_hist.hsize(base)
		base = base << 1

	for (i, v) in hist.items():
		print "%s\t%d\t%d" % (units[i], i, v)
