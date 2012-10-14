/*
 * ===========================================================================
 *
 *       Filename:  RequestGenerator.h
 *         Author:  Ming Chen, v.minchen@gmail.com
 *        Created:  10/13/2012 02:38:45 AM
 *
 *    Description:  Generate Image Requests
 *
 *       Revision:  none
 *
 *
 * ===========================================================================
 */

#ifndef MRIS_REQUEST_GENERATOR_H_
#define MRIS_REQUEST_GENERATOR_H_

#include <vector>
#include <stdlib.h>
#include <assert.h>

namespace mris {

class Request {
private:
	int size;			// Request size in bytes
	double frequency;	// Relative frequency of requests of this size
public:
	Request(int size, double freq) : size(size), frequency(freq) {}
};

class RequestGenerator {
private:
	std::vector<Request> requests;	// different types of requests
	double totalFrequency;
public:
	RequestGenerator() : totalFrequency(0) {}
	RequestGenerator(const Request &req1, const Request &req2) {
		registorReqeust(req1);
		registorReqeust(req2);
	}
	registorReqeust(const Request &req) {
		requests.push_back(req);
		totalFrequency += req.frequency;
	}
	int generate() {
		double r = totalFrequency * drand48();
		size_t i;
		for (i = 0; i < requests.size(); ++i) {
			if (r < requests[i].frequency)
				return requests[i].size;
			r -= requests[i].frequency;
		}
		assert(false);
		return 0;
	}
};

}

#endif
