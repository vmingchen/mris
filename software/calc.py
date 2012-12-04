#!/usr/bin/python
import math
import sys
import itertools

class Calculator:
	@staticmethod
	def get_mean(dataset):
		sum = 0
		cnt = 0
		for i in dataset:
			sum += i
			cnt += 1
		return sum / cnt

	@staticmethod
	def get_stdv(dataset, mean):
		dv = 0
		cnt = 0
		for i in dataset:
			dv += (i - mean) * (i - mean)
			cnt += 1
		stdv = math.sqrt(dv)
		var = 2 * stdv / cnt
		return var

	@staticmethod
	def get_meanstdv(dataset):
		ds1, ds2 = itertools.tee(dataset)
		mean = Calculator.get_mean(ds1)
		stdv = Calculator.get_stdv(ds2, mean)
		return (mean, stdv)

_usage = '''
usage: calc.py -[s|f|p|h] [filename|parameters]
	-s	read from stdin (calc.py -s)
	-f	read from file (calc.py -f filename)
	-p  read from parameters (calc.py -p v1 v2 v3 ...)
	-h  get help
'''


if __name__ == '__main__':
	if len(sys.argv) == 1:
		print _usage
		sys.exit(1)

	if sys.argv[1] == '-s':
		# read from stdin
		print "%.2f\t%.2f" % Calculator.get_meanstdv((float(i) for i in sys.stdin))
	elif sys.argv[1] == '-f':
		# read from file
		print "%.2f\t%.2f" % Calculator.get_meanstdv((float(i) for i in open(sys.argv[2])))
	elif sys.argv[1] == '-p':
		# read from parameters
		print "%.2f\t%.2f" % Calculator.get_meanstdv((float(i) for i in sys.argv[2:]))
	elif sys.argv[1] == '-h':
		print _usage
		sys.exit(0)
	else:
		print _usage
		sys.exit(1)
