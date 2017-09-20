all: Preprocess

CXXFLAGS+= -I ../include/ -std=gnu++11

Preprocess: Preprocess.cc 
	$(CXX) $(CXXFLAGS) $@.cc -o ../bin/$@
.PHONY: clean
clean:
	rm datagen
