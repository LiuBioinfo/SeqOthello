#pragma once
#include "othello.h"
#include <map>
#include <string>

using namespace std;

class L1Node {
    void constructothello(uint32_t, uint32_t, string);
private:
    uint32_t splitbit;
    uint32_t shift;
public:
    uint32_t kmerLength;
    vector<Othello<uint64_t> *> othellos;
    vector<vector<uint64_t> *> kV;
    vector<vector<uint16_t> *> vV;
    uint32_t grpidlimit;
    constexpr static uint64_t L1Partlimit = 1048576*128;
    constexpr static uint64_t L1InQlimit = 1048576*512;
    L1Node() {}
    L1Node(uint64_t estimatedKmerCount, int _kmerlength) : kmerLength(_kmerlength) {
        splitbit = 0;
        while( (estimatedKmerCount >> splitbit) > L1Partlimit)  splitbit ++;

        if (splitbit >= kmerLength)
            throw std::invalid_argument("invalid parameter for L1Node");
        setsplitbit(kmerLength, splitbit);
        grpidlimit = (1<< splitbit);
        kV.clear();
        vV.clear();
        for (unsigned int i = 0 ; i < grpidlimit; i++) {
            kV.push_back(new vector<uint64_t>());
            vV.push_back(new vector<uint16_t>());
        }
        othellos.resize(grpidlimit);
    }

    uint64_t queryInt(uint64_t k);
    void add(uint64_t &k, uint16_t v);
    void writeToFile(string fname);
    ~L1Node();
    void constructAndWrite(uint32_t, uint32_t, string);
    void loadFromFile(string fname);
    void putInfoToXml(tinyxml2::XMLElement *, string);
    void setsplitbit(uint32_t _kmerlength, uint32_t t) {
        kmerLength = _kmerlength;
        splitbit = t;
        shift = kmerLength*2 - splitbit;
    }
    uint32_t getsplitbit() {
        return splitbit;
    }
    map<int, double> printrates();
};


