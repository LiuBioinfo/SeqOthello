#include "L2Node.hpp"


inline uint8_t get4b(uint8_t **pp, bool &hasvalue, uint8_t &buff) {
   if (hasvalue) {
       hasvalue = false;
       (*pp)++;
       return buff;
   }
   else {
      buff = ((**pp)>>4);
      hasvalue = true;
      return (**pp) & 0xF;
   }
}
inline uint8_t put4b(uint8_t **pp, bool &filledhalf, uint8_t val) {
   if (filledhalf) {
       (**pp) |= ((val & 0xF)<<4);
       (*pp)++;
       filledhalf = false;
   } else {
       (**pp) = (val & 0xF);
       filledhalf = true;
   }
}

uint32_t valuelistDecode(uint8_t *p, vector<uint32_t> &val, uint32_t maxmem) {
     uint8_t **pp = &p;
     val.clear();
     bool hasvalue = false; uint8_t buff = 0;
     while ( *pp < (p+maxmem) ) {
         uint8_t v = get4b(pp, hasvalue, buff);
         if (v & 0x8) {
             if (v == 8) return val.size();
             val.push_back(v&0x7);
             continue;
         }
         uint32_t v2 = get4b(pp, hasvalue, buff);
         if (v & 0x4) {
             uint32_t a = (v & 0x3);
             val.push_back( (a<<4) | v2);
             continue;
         }
         uint32_t v3 = get4b(pp, hasvalue, buff);
         if (v & 0x2) {
             uint32_t a = (v & 0x1);
             val.push_back( (a<<8) | (v2<<4) | v3);
             continue;
         }
         uint32_t v4 = get4b(pp, hasvalue, buff);
         if (v == 1) {
             val.push_back( (v2<<8) | (v3<<4) | v4);
             continue;
         }
         uint32_t v5 = get4b(pp, hasvalue, buff);
         uint32_t v6 = get4b(pp, hasvalue, buff);
         val.push_back( (v2<<16) | (v3<<12) | (v4<<8) | (v5<<4) | v6);
     }
     return val.size();
}

uint32_t valuelistEncode(uint8_t *p, vector<uint32_t> &val, bool really) {
    uint32_t ans = 0;
    
    uint8_t **pp = &p;
    uint8_t *p0 = p;
    bool filledhalf = false;
    //inline uint8_t put4b(uint8_t **pp, bool &filledhalf, uint8_t val) {
    for (auto &x: val) {
        if (x>0xFFF) { //>12bits
           if (really) {
           put4b(pp, filledhalf, 0);
           put4b(pp, filledhalf, 0xF & (x >> 16));
           put4b(pp, filledhalf, 0xF & (x >> 12));
           put4b(pp, filledhalf, 0xF & (x >> 8));
           put4b(pp, filledhalf, 0xF & (x >> 4));
           put4b(pp, filledhalf, 0xF & x);
           }
           else ans += 6;
        }
        else if (x>0x1FF) { //10~12bits
           if (really) {
           put4b(pp, filledhalf, 1);
           put4b(pp, filledhalf, 0xF & (x >> 8));
           put4b(pp, filledhalf, 0xF & (x >> 4));
           put4b(pp, filledhalf, 0xF & x);
           }
           else ans += 4;
        }
        else if (x>0x3F) { // 7~9 bits
           if (really) {
           put4b(pp, filledhalf, 2 | (x>>8) );
           put4b(pp, filledhalf, 0xF & (x >> 4));
           put4b(pp, filledhalf, 0xF & x);
           }
           else ans += 3;
        }
        else if (x>0x7) { // 4~6 bits
           if (really) {
           put4b(pp, filledhalf, 4 | (x>>4) );
           put4b(pp, filledhalf, 0xF & x);
           } 
           else ans += 2;
        }
        else { //<=3bits
           if (really)
               put4b(pp, filledhalf, 0x8 | x);
           else 
               ans ++;
        }
    }
    if (really) {
        put4b(pp, filledhalf, 8);
        return (p-p0) + (filledhalf);
    }
    ans ++;
    return (ans>>1) + (ans & 1);
}

void L2Node::constructOth() {
    int L = 8;
    while ((1<<L)<entrycnt+10) L++;
    printf("construct L2Node with %lld keys. Using type %s. with %d-Othello, entrycnt %d.\n", keys.size(), L2NodeTypes::typestr.at(this->getType()).c_str(), L, entrycnt);
    oth = new Othello<keyType> (L, keys,values, true, 0);
    for (auto &k: oth->removedKeys) {
        printf("Removed key vNode %llx\n", k);
    }
}



bool L2ShortValueListNode::smartQuery(const keyType *k, vector<uint32_t> &ret, vector<uint8_t> &retmap) {
    uint64_t index = L2Node::oth->queryInt(*k);
    ret.clear();
    if (index >= uint64list.size()) return true;
    uint64_t vl = uint64list[index];
    uint32_t valcnt = valuecnt;
    while (valcnt -- ) {
        uint32_t pq = vl & mask;
        vl >>= maxnl;
        ret.push_back(pq);
    }
    return true;
}
bool L2EncodedValueListNode::smartQuery(const keyType *k, vector<uint32_t> &ret, vector<uint8_t> &retmap) {
     uint64_t index = L2Node::oth->queryInt(*k);
     if (encodetype == L2NodeTypes::VALUE_INDEX_ENCODED) {
        vector<uint32_t> decode;
        valuelistDecode(&lines[IOLengthInBytes*index], decode, IOLengthInBytes);
        ret.clear();
        uint32_t last; 
        ret.push_back(last = decode[0]);
        for (int i = 1; i< decode.size(); i++) {
            last += decode[i];
            ret.push_back(last);
        }
        return true;
     }
     else {
        //MAPP
        retmap = vector<uint8_t> (lines.begin()+IOLengthInBytes * index , lines.begin() + IOLengthInBytes * (index+1));
        return false;
     }
}

void L2ShortValueListNode::add(keyType &k, vector<uint32_t> & valuelist) {
    keycnt++;
    uint64_t value = 0ULL;
    for (auto pval = valuelist.rbegin(); pval!=valuelist.rend(); pval++) {
        value <<= maxnl;
        value |= (*pval & mask);
    }
    if (valuemap.count(value) == 0) {
        //we always prepend one  to avoid \tau result = 0;
        if (uint64list.size() ==0)
            uint64list.push_back(value);

        valuemap[value] = valuemap.size();
        uint64list.push_back(value);
		entrycnt = uint64list.size();
    }
    values.push_back(valuemap[value]);
    keys.push_back(k);
    return;

}

void L2EncodedValueListNode::add(keyType &k, vector<uint32_t> & valuelist) { // this valuelist is diff.
   if (encodetype!= L2NodeTypes::VALUE_INDEX_ENCODED)
       throw invalid_argument("can not add value list L2EncodedValueListNode");
    keys.push_back(k);
    vector<uint8_t> buff(IOLengthInBytes);
    if (lines.size() == 0)
         lines.resize(IOLengthInBytes);
    uint32_t curr = lines.size();
    lines.resize(IOLengthInBytes + curr);
    valuelistEncode(&lines[curr], valuelist, true);
    keycnt++;
    entrycnt++;
    values.push_back(keycnt);

}

void L2EncodedValueListNode::addMAPP(keyType &k, vector<uint8_t> &mapp) {
   if (encodetype!= L2NodeTypes::MAPP)
       throw invalid_argument("can not add bitmap to L2EncodedValueListNode");
   keys.push_back(k);
   values.push_back(keycnt);
   keycnt++;
   entrycnt++;
   if (mapp.size() != IOLengthInBytes) {
       throw invalid_argument("can not add bitmap to L2ShortValuelist type");
   }
   lines.insert(lines.end(), mapp.begin(), mapp.end());
}

void L2ShortValueListNode::writeDataToGzipFile(gzFile fout) {
    unsigned char buf[0x20];
    memset(buf,0,sizeof(buf));
    memcpy(buf, &valuecnt, 4);
    memcpy(buf+4, &maxnl, 4);
    uint32_t siz = uint64list.size();
    memcpy(buf+8, &siz, 4);
    gzwrite(fout, buf,sizeof(buf));
    L2Node::oth->exportInfo(buf);
    gzwrite(fout, buf,sizeof(buf));
    L2Node::oth->writeDataToGzipFile(fout);
    for (auto const & vl: uint64list) {
        uint64_t rvl = vl;
        gzwrite(fout, &rvl, IOLengthInBytes);
    }
}

void L2EncodedValueListNode::writeDataToGzipFile(gzFile fout) {
    unsigned char buf[0x20];
    memset(buf,0,sizeof(buf));
    memcpy(buf, &IOLengthInBytes, 4);
    memcpy(buf+4, &encodetype, 4);
    uint32_t siz = lines.size();
    memcpy(buf+8, &siz, 4);
    gzwrite(fout, buf,sizeof(buf));
    L2Node::oth->exportInfo(buf);
    gzwrite(fout, buf,sizeof(buf));
    L2Node::oth->writeDataToGzipFile(fout);
    gzwrite(fout, &lines[0], lines.size());
}


void L2ShortValueListNode::loadDataFromGzipFile(gzFile fin) {
    unsigned char buf[0x20];
    gzread(fin, buf,sizeof(buf));
    void *p;
    p = &buf;
    memcpy(&valuecnt, p, 4);
    memcpy(&maxnl, p+0x4, 4);
    uint32_t siz;
    memcpy(&siz, p+0x8, 4);

    gzread(fin, buf,sizeof(buf));
    L2Node::oth = new Othello<uint64_t> (buf);
    L2Node::oth->loadDataFromGzipFile(fin);
    uint64list.resize(0);//ShortVLcount);
    for (int i = 0 ; i < siz; i++) {
        uint64_t vl = 0ULL;
        gzread(fin, &vl, IOLengthInBytes);
        uint64list.push_back(vl);
    }
}


void L2EncodedValueListNode::loadDataFromGzipFile(gzFile fin) {
    unsigned char buf[0x20];
    gzread(fin, buf,sizeof(buf));
    void *p;
    p = &buf;
    memcpy(&IOLengthInBytes, p, 4);
    memcpy(&encodetype, p+0x4, 4);
    uint32_t siz;
    memcpy(&siz, p+0x8, 4);

    gzread(fin, buf,sizeof(buf));
    L2Node::oth = new Othello<uint64_t> (buf);
    L2Node::oth->loadDataFromGzipFile(fin);
    lines.resize(siz);//ShortVLcount);
    gzread(fin, &lines[0], siz);
}

