// This file is a part of SeqOthello. Please refer to LICENSE.TXT for the LICENSE
#pragma once
#include "othello.h"
#include <map>
#include <string>
#include <threadpool.h>

using namespace std;

class L1Node {
    void constructothello(uint32_t, uint32_t, string);
private:
    uint32_t splitbit;
    uint32_t shift;
    string fname;
public:
    uint32_t kmerLength;
    vector<Othello<uint64_t> *> othellos;
    vector<IOBuf<uint64_t> *> kV;
    vector<IOBuf<uint16_t> *> vV;
    uint32_t grpidlimit;
    constexpr static uint64_t L1Partlimit = 1048576*128;
    constexpr static uint64_t L1InQlimit = 1048576*512;
    L1Node() {}
    L1Node(uint64_t estimatedKmerCount, int _kmerlength, const string &buf) : kmerLength(_kmerlength) {
        splitbit = 0;
        while( (estimatedKmerCount >> splitbit) > L1Partlimit)  splitbit ++;

        if (splitbit >= kmerLength)
            throw std::invalid_argument("invalid parameter for L1Node");
        setsplitbit(kmerLength, splitbit);
        grpidlimit = (1<< splitbit);
        kV.clear();
        vV.clear();
        for (unsigned int i = 0 ; i < grpidlimit; i++) {
            stringstream ss;
            ss << buf << i;
            string fstr;
            ss >> fstr;
            kV.push_back(new IOBuf<uint64_t>((fstr+".keys").c_str()));
            vV.push_back(new IOBuf<uint16_t>((fstr+".values").c_str()));
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
    void setfname(string);
    void queryPartAndPutToVV(vector<vector<uint16_t>> &ans, vector<vector<uint64_t>> &kmers, int grp, int threads);
};


