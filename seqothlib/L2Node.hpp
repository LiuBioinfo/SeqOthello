#pragma once
#include <string>
#include <map>
#include <vector>
#include <zlib.h>
#include "othello.h"
#include "util.h"
#include <tinyxml2.h>
#include <memory>

using namespace std;

uint32_t valuelistEncode(uint8_t *, vector<uint32_t> &val, bool really); //return encode length in byte.

uint32_t valuelistDecode(uint8_t *, vector<uint32_t> &val, uint32_t maxmem);

typedef uint64_t keyType;
namespace L2NodeTypes {
static const int VALUE_INDEX_SHORT = 16;
static const int VALUE_INDEX_ENCODED = 17;
static const int MAPP = 4;
const map<int, string> typestr= { {VALUE_INDEX_SHORT, "ShortValueList"}, {VALUE_INDEX_ENCODED,"EncodedValueList"}, {MAPP,"Bitmap"}};
};

class L2Node {
public:
    virtual int getType() = 0;
    virtual bool smartQuery(const keyType *k, vector<uint32_t> &ret, vector<uint8_t> &retmap) = 0;
    virtual void add(keyType &k, vector<uint32_t> &) = 0;
    virtual void addMAPP(keyType &k, vector<uint8_t> &mapp) = 0;
    virtual void writeDataToGzipFile(gzFile fout) = 0;
    virtual void loadDataFromGzipFile(gzFile fin) = 0;
    uint32_t keycnt = 0;
    vector<uint32_t> values;
    vector<uint64_t> keys;
    void constructOth();
    uint32_t entrycnt = 0;
    Othello<uint64_t> *oth = NULL;
    virtual void putInfoToXml(tinyxml2::XMLElement *) = 0;
    static std::shared_ptr<L2Node> loadL2Node( tinyxml2::XMLElement *p);
};

class L2ShortValueListNode : public L2Node {
    vector<uint64_t> uint64list;
    uint32_t valuecnt, maxnl, mask;
    map<uint64_t, uint32_t> valuemap;
    uint32_t IOLengthInBytes;
    void definetypes() {
        mask = (1<<maxnl);
        mask--;
        IOLengthInBytes = maxnl*valuecnt /8;
        while (IOLengthInBytes *8 < maxnl*valuecnt)
            IOLengthInBytes++;
    }
public:
    int getType() override {
        return L2NodeTypes::VALUE_INDEX_SHORT;
    }
    L2ShortValueListNode(uint32_t _valuecnt, uint32_t _maxnl) : valuecnt(_valuecnt), maxnl(_maxnl) {
        definetypes();
    }

    bool smartQuery(const keyType *k, vector<uint32_t> &ret, vector<uint8_t> &retmap) override;
    void add(keyType &k, vector<uint32_t> &) override;
    void addMAPP(keyType &, vector<uint8_t> &) override {
        throw invalid_argument("can not add bitmap to L2ShortValuelist type");
    }
    void writeDataToGzipFile(gzFile fout) override;
    void loadDataFromGzipFile(gzFile fin) override;
    void putInfoToXml(tinyxml2::XMLElement *) override;
};

class L2EncodedValueListNode : public L2Node {
    vector<uint8_t> lines;
    uint32_t IOLengthInBytes, encodetype;
    uint32_t keycnt  = 0;
public:
    int getType() override {
        return encodetype;
    }
    L2EncodedValueListNode(uint32_t _IOLengthInBytes, uint32_t _encodetype) :  IOLengthInBytes(_IOLengthInBytes), encodetype(_encodetype) {
        if (encodetype != L2NodeTypes::MAPP && encodetype!= L2NodeTypes::VALUE_INDEX_ENCODED)
            throw invalid_argument("can not add bitmap to L2ShortValuelist type");
    }

    bool smartQuery(const keyType *k, vector<uint32_t> &ret, vector<uint8_t> &retmap) override;
    void add(keyType &k, vector<uint32_t> &) override;
    void addMAPP(keyType &k, vector<uint8_t> &mapp) override;
    void writeDataToGzipFile(gzFile fout) override;
    void loadDataFromGzipFile(gzFile fin) override;
    void putInfoToXml(tinyxml2::XMLElement *) override;
};

