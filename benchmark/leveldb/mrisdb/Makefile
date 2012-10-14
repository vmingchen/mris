CXXFLAGS += -Wall -Werror -O2 -I../leveldb/include -I../leveldb
#CFLAGS = -Wall -Werror -g O0

all: twodb

twodb: SeparateDB.cpp
	$(CXX) $(CXXFLAGS) -o $@ SeparateDB.cpp -lleveldb

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@
