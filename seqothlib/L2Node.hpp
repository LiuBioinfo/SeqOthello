#pragma once
#include <string>
#include <map>
#include <vector>
using namespace std;

uint32_t valuelistEncode(uint8_t *, vector<uint32_t> &val, bool really);

uint32_t valuelistDecode(uint8_t *, vector<uint32_t> &val, uint32_t maxmem); //return encode length in 'half byte';

class vNodeNew {
    static const int VALUE_INDEX_SHORT = 16;
    static const int VALUE_INDEX_LONG = 17;
    
    static const int MAPP = 4;
    const std::map<int, std::string> typestr= { {VALUE_INDEX_SHORT, "ShortValueList"}, {VALUE_INDEX_LONG,"LongValuelist"}, {MAPP,"Bitmap"}};
public:
    uint32_t high;
    uint32_t type;
 
};
