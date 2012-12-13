#!/usr/bin/python
'''
Modeling the read workload.
REQUIRE: mris_ratio_ops.dat

Reference: 
http://glowingpython.blogspot.com/2012/03/linear-regression-with-numpy.html
'''
from numpy import arange,array,ones,linalg,repeat
from pylab import plot,show

def get_ops(setup):
	'''Get the ops/sec of @setup from data file'''
	opsind = {'ssd': 2, 'hybrid': 5, 'sata': 8}[setup]
	fdata = open('mris_ratio_ops.dat')
	ops = array([float(ln.split()[opsind]) for ln in fdata if ln[0] != '#'])
	return ops

def get_ratio():
	fdata = open('mris_ratio_ops.dat')
	return array([float(ln.split()[0]) for ln in fdata if ln[0] != '#'])

def plot_model(x, y, w):
	# plot regression
	line = w[0]*x+w[1] # regression line
	plot(x, line, 'r-', x, y, 'o')
	show()

def model(setup):
	x = get_ratio()			# ratio
	n = len(x)
	A = array([x, ones(n)])
	ops = get_ops(setup)
	# t_s * ratio + t_l = 1000000*(ratio + 1)/ops
	y = repeat(1000000, n) * (x + repeat(1, n)) / ops
	w = linalg.lstsq(A.T, y)[0]
	# plot_model(x, y, w)
	return w

if __name__ == '__main__':
	print model('ssd')
	print model('sata')
	print model('hybrid')
