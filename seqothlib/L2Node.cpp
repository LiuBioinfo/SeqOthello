// This file is a part of SeqOthello. Please refer to LICENSE.TXT for the LICENSE
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
inline void put4b(uint8_t **pp, bool &filledhalf, uint8_t val) {
    if (filledhalf) {
        (**pp) |= ((val & 0xF)<<4);
        (*pp)++;
        filledhalf = false;
    } else {
        (**pp) = (val & 0xF);
        filledhalf = true;
    }
}
inline uint32_t getvalue(uint8_t **pp, bool & hasvalue, uint8_t &buff, bool & finished) {
    uint8_t v = get4b(pp, hasvalue, buff);
    if (v & 0x8) {
        if (v == 8) finished = true;
        return (v&0x7);
    }
    uint32_t v2 = get4b(pp, hasvalue, buff);
    if (v & 0x4) {
        uint32_t a = (v & 0x3);
        return ( (a<<4) | v2);
    }
    uint32_t v3 = get4b(pp, hasvalue, buff);
    if (v & 0x2) {
        uint32_t a = (v & 0x1);
        return ( (a<<8) | (v2<<4) | v3);
    }
    uint32_t v4 = get4b(pp, hasvalue, buff);
    if (v == 1) {
        return (v2<<8) | (v3<<4) | v4;
    }
    uint32_t v5 = get4b(pp, hasvalue, buff);
    uint32_t v6 = get4b(pp, hasvalue, buff);
    return ( (v2<<16) | (v3<<12) | (v4<<8) | (v5<<4) | v6);
}
uint32_t valuelistDecode(uint8_t *p, vector<uint32_t> &val, uint32_t maxmem) {
    uint8_t **pp = &p;
    val.clear();
    bool hasvalue = false;
    uint8_t buff = 0;
    bool finished = false;;
    while ( *pp < (p+maxmem) ) {
        uint32_t x = getvalue(pp, hasvalue, buff,finished);
        if (finished)
            return val.size();
        val.push_back(x);
        if (x == 1) {
            uint32_t dup = getvalue(pp, hasvalue, buff, finished);
            while (dup > 1) {
                val.push_back(1);
                dup--;
            }
        }
    }
    return val.size();
}
inline void putvalue(uint32_t x, uint8_t **pp, bool &filledhalf, bool really, uint32_t & ans) {
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
    else if (x>0x7 || x==0) { // 4~6 bits or 0
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
uint32_t valuelistEncode(uint8_t *p, vector<uint32_t> &val, bool really) {
    uint32_t ans = 0;

    uint8_t **pp = &p;
    uint8_t *p0 = p; //starting location
    bool filledhalf = false;
    //inline uint8_t put4b(uint8_t **pp, bool &filledhalf, uint8_t val) {
    for (unsigned int i = 0 ; i < val.size();) {
        auto x = val[i];
        putvalue(x, pp, filledhalf, really, ans);
        if (x==1) {
            unsigned int j = i;
            while (val[j] == 1) {
                j++;
                if (j == val.size()) break;
            }
            putvalue(j-i, pp, filledhalf, really, ans);
            i=j;
        }
        else
            i++;
    }
    if (really) {
        put4b(pp, filledhalf, 8);
        return (p-p0) + (filledhalf);
    }
    ans ++;
    return (ans>>1) + (ans & 1);
}

void L2Node::constructOth() {
    uint32_t L = 8;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
    while ((1<<L)<entrycnt+10) L++;
#pragma GCC diagnostic pop
    printf("%s: construct L2Node with %lu keys. Using type %s. with %u-Othello, entrycnt %u.\n", get_thid().c_str(), keys.size(), L2NodeTypes::typestr.at(this->getType()).c_str(), L, entrycnt);
    oth = new Othello<keyType> (L, keys,values, true, 0);
    for (auto &k: oth->removedKeys) {
        printf("%s: Removed key vNode %lx\n", get_thid().c_str(), k);
    }
    keys.clear();
    values.clear();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
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
#pragma GCC diagnostic pop

bool L2EncodedValueListNode::smartQuery(const keyType *k, vector<uint32_t> &ret, vector<uint8_t> &retmap) {
    uint64_t index = L2Node::oth->queryInt(*k);
    if (encodetype == L2NodeTypes::VALUE_INDEX_ENCODED) {
        ret.clear();
        if (IOLengthInBytes*index >= lines.size()) return true;
        if (index==0) return true;
        vector<uint32_t> decode;
        valuelistDecode(&lines[IOLengthInBytes*index], decode, IOLengthInBytes);
        if (decode.size()==0) return true;
        uint32_t last;
        ret.push_back(last = decode[0]);
        for (uint32_t i = 1; i< decode.size(); i++) {
            last += decode[i];
            ret.push_back(last);
        }
        return true;
    }
    else {
        //MAPP
        if (IOLengthInBytes*index >= lines.size()) {
            ret.clear();
            return true;
        }
        retmap = vector<uint8_t> (lines.begin()+IOLengthInBytes * index , lines.begin() + IOLengthInBytes * (index+1));
        return false;
    }
}

void L2ShortValueListNode::add(keyType &k, vector<uint32_t> & valuelist) {
    if (fdata==NULL) {
        fdata = gzopen((gzfname+".dat").c_str(),"wb");
//        gzbuffer(fdata,64*1024);
        if (fdata == NULL) {
            fprintf(stderr,"failed to open file %s to write\n", (gzfname+".dat").c_str());
            return;
        }
    }
    keycnt++;
    uint64_t value = 0ULL;
    for (auto pval = valuelist.rbegin(); pval!=valuelist.rend(); pval++) {
        value <<= maxnl;
        value |= (*pval & mask);
    }
    if (valuemap.count(value) == 0) {
        //we always prepend one  to avoid \tau result = 0;
        if (siz == 0) {
            uint64_t u0 = 0;
            siz ++;
            gzwrite(fdata, &u0, IOLengthInBytes);
        }
        //}
        valuemap[value] = siz;
        gzwrite(fdata, &value, IOLengthInBytes);
        siz++;
        entrycnt = siz;
    }
    values.push_back(valuemap[value]);
    keys.push_back(k);
    return;

}

void L2EncodedValueListNode::add(keyType &k, vector<uint32_t> & valuelist) { // this valuelist is diff.
    vector<uint8_t> buff(IOLengthInBytes,0);
    if (encodetype!= L2NodeTypes::VALUE_INDEX_ENCODED)
        throw invalid_argument("can not add value list L2EncodedValueListNode");
    if (fdata==NULL) {
        fdata = gzopen((gzfname+".dat").c_str(),"wb");
//        gzbuffer(fdata,64*1024);
        if (fdata == NULL) {
            fprintf(stderr,"failed to open file %s to write\n", (gzfname+".dat").c_str());
            return;
        }
    }
    keys.push_back(k);
    keycnt++;
    valuelistEncode(&buff[0], valuelist, true);
    if (IOLengthInBytes<=8) {
        uint64_t v64 = 0;
        memcpy(&v64, &buff[0], IOLengthInBytes);
        if (valuemap.count(v64) ==0) {
            if (siz == 0) {
                siz += IOLengthInBytes;
                entrycnt++;
                gzwrite(fdata,&buff[0], IOLengthInBytes);
            }
            valuemap[v64] = entrycnt;
            entrycnt++;
            siz += IOLengthInBytes;
            gzwrite(fdata,&buff[0], IOLengthInBytes);
        }
        values.push_back(valuemap[v64]);
    }
    else {
        if (siz == 0) { //lines.size() == 0) {
            //lines.resize(IOLengthInBytes);
            siz += IOLengthInBytes;
            entrycnt++;
            gzwrite(fdata,&buff[0], IOLengthInBytes);
        }
        //uint32_t curr = lines.size();
        //lines.resize(lines.size() + IOLengthInBytes);
        siz += IOLengthInBytes;
        entrycnt++;
        gzwrite(fdata,&buff[0], IOLengthInBytes);
        values.push_back(keycnt);
    }

}

void L2EncodedValueListNode::addMAPP(keyType &k, vector<uint8_t> &mapp) {
    if (encodetype!= L2NodeTypes::MAPP)
        throw invalid_argument("can not add bitmap to L2EncodedValueListNode");
    if (fdata==NULL) {
        fdata = gzopen((gzfname+".dat").c_str(),"wb");
//        gzbuffer(fdata,256*1024);
        if (fdata == NULL) {
            fprintf(stderr,"failed to open file %s to write\n", (gzfname+".dat").c_str());
            return;
        }
    }
    keys.push_back(k);
    //TODO :: Need to pre-pend a record for false positives!
    keycnt++;
    entrycnt++;
    if (siz == 0) { //lines.size() == 0) {
        //lines.resize(mapp.size());
        siz += IOLengthInBytes;
        vector<uint8_t> buff(IOLengthInBytes);
        gzwrite(fdata,&buff[0], IOLengthInBytes);
        entrycnt++;
    }
    if (mapp.size() != IOLengthInBytes) {
        throw invalid_argument("can not add bitmap to L2ShortValuelist type");
    }
    //uint32_t curr = lines.size();
    //lines.insert(lines.end(), mapp.begin(), mapp.end());
    gzwrite(fdata,&mapp[0], IOLengthInBytes);
    values.push_back(keycnt);
    siz += IOLengthInBytes;
}

void L2ShortValueListNode::writeDataToGzipFile() {
    printf("%s : writing to L2 Gzip File %s\n", get_thid().c_str(), gzfname.c_str());
    gzFile fout = gzopen(gzfname.c_str(), "wb");
    unsigned char buf[0x20];
    memset(buf,0,sizeof(buf));
    memcpy(buf, &valuecnt, 4);
    memcpy(buf+4, &maxnl, 4);
    memcpy(buf+8, &siz, 4);
    gzwrite(fout, buf,sizeof(buf));
    L2Node::oth->exportInfo(buf);
    gzwrite(fout, buf,sizeof(buf));
    L2Node::oth->writeDataToGzipFile(fout);
    gzclose(fout);
    uint64list.clear();
    delete L2Node::oth;
    //for (auto const & vl: uint64list) {
    //  uint64_t rvl = vl;
    //  gzwrite(fdata, &rvl, IOLengthInBytes);
    //}
    gzclose(fdata);
}

void L2EncodedValueListNode::writeDataToGzipFile() {
    printf("%s: Write L2 Node %s\n", get_thid().c_str(), gzfname.c_str());
    gzFile fout = gzopen(gzfname.c_str(), "wb");
    unsigned char buf[0x20];
    memset(buf,0,sizeof(buf));
    memcpy(buf, &IOLengthInBytes, 4);
    memcpy(buf+4, &encodetype, 4);
    memcpy(buf+8, &siz, 4);
    gzwrite(fout, buf,sizeof(buf));
    L2Node::oth->exportInfo(buf);
    gzwrite(fout, buf,sizeof(buf));
    L2Node::oth->writeDataToGzipFile(fout);
    //gzwrite(fdata, &lines[0], lines.size());
    gzclose(fout);
    gzclose(fdata);
}


void L2ShortValueListNode::loadDataFromGzipFile() {
    printf("%s: Load L2 Node %s\n", get_thid().c_str(), gzfname.c_str());
    gzFile fin = gzopen(gzfname.c_str(), "rb");
//    gzbuffer(fin,256*1024);
    unsigned char buf[0x20];
    memset(buf,0,sizeof(buf));
    gzread(fin, buf,sizeof(buf));
    void *p;
    p = &buf;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
    memcpy(&valuecnt, p, 4);
    memcpy(&maxnl, p+0x4, 4);
    uint32_t siz;
    memcpy(&siz, p+0x8, 4);
#pragma GCC diagnostic pop
    gzread(fin, buf,sizeof(buf));
    L2Node::oth = new Othello<uint64_t> (buf);
    L2Node::oth->loadDataFromGzipFile(fin);
    gzFile fin2 = gzopen((gzfname+".dat").c_str(), "rb");
//    gzbuffer(fin2,256*1024);
    uint64list.resize(0);//ShortVLcount);
    for (uint32_t i = 0 ; i < siz; i++) {
        uint64_t vl = 0ULL;
        gzread(fin2, &vl, IOLengthInBytes);
        uint64list.push_back(vl);
    }
    gzclose(fin);
    gzclose(fin2);
}


void L2EncodedValueListNode::loadDataFromGzipFile() {
    printf("%s: Load L2 Node %s\n", get_thid().c_str(), gzfname.c_str());
    gzFile fin = gzopen(gzfname.c_str(), "rb");
//    gzbuffer(fin,256*1024);
    unsigned char buf[0x20];
    memset(buf,0,sizeof(buf));
    gzread(fin, buf,sizeof(buf));
    void *p;
    p = &buf;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
    memcpy(&IOLengthInBytes, p, 4);
    memcpy(&encodetype, p+0x4, 4);
    uint32_t siz;
    memcpy(&siz, p+0x8, 4);
#pragma GCC diagnostic pop
    gzread(fin, buf,sizeof(buf));
    L2Node::oth = new Othello<uint64_t> (buf);
    L2Node::oth->loadDataFromGzipFile(fin);
    lines.resize(siz);//ShortVLcount);
    gzFile fin2 = gzopen((gzfname+".dat").c_str(), "rb");
//    gzbuffer(fin2,256*1024);
    gzread(fin2, &lines[0], siz);
    gzclose(fin);
    gzclose(fin2);
}

void L2ShortValueListNode::putInfoToXml(tinyxml2::XMLElement * pe) {
    string typestr = L2NodeTypes::typestr.at(this->getType());
    pe->SetAttribute("Type", typestr.c_str());
    pe->SetAttribute("ValueCnt", valuecnt);
    pe->SetAttribute("BitsPerValue", maxnl);
    pe->SetAttribute("Keycount", keycnt);
    pe->SetAttribute("EntryCount", entrycnt);
    pe->SetAttribute("L2FileName", gzfname.c_str());
}

void L2EncodedValueListNode::putInfoToXml(tinyxml2::XMLElement *pe) {
    string typestr = L2NodeTypes::typestr.at(this->getType());
    pe->SetAttribute("Type", typestr.c_str());
    pe->SetAttribute("IOLengthInBytes", IOLengthInBytes);
    pe->SetAttribute("Keycount", keycnt);
    pe->SetAttribute("EntryCount", entrycnt);
    pe->SetAttribute("L2FileName", gzfname.c_str());
}


std::shared_ptr<L2Node>
L2Node::createL2Node( tinyxml2::XMLElement *p) {
    std::shared_ptr<L2Node> ptr(NULL);
    string fname(p->Attribute("L2FileName"));
    if (strcmp(p->Attribute("Type"), L2NodeTypes::typestr.at(L2NodeTypes::VALUE_INDEX_SHORT).c_str()) == 0) {
        int valuecnt = p->IntAttribute("ValueCnt");
        int maxnl = p->IntAttribute("BitsPerValue");
        int entrycnt = p->IntAttribute("EntryCount");
        if (entrycnt)
            ptr = make_shared<L2ShortValueListNode>(valuecnt, maxnl,fname);
    }

    if (strcmp(p->Attribute("Type"), L2NodeTypes::typestr.at(L2NodeTypes::VALUE_INDEX_ENCODED).c_str()) == 0) {
        int IOL = p->IntAttribute("IOLengthInBytes");
        int type = L2NodeTypes::VALUE_INDEX_ENCODED;
        int entrycnt = p->IntAttribute("EntryCount");
        if (entrycnt)
            ptr =  make_shared<L2EncodedValueListNode>(IOL, type,fname);
    }

    if (strcmp(p->Attribute("Type"), L2NodeTypes::typestr.at(L2NodeTypes::MAPP).c_str()) == 0) {
        int IOL = p->IntAttribute("IOLengthInBytes");
        int type = L2NodeTypes::MAPP;
        int entrycnt = p->IntAttribute("EntryCount");
        if (entrycnt)
            ptr = make_shared<L2EncodedValueListNode>(IOL, type,fname);
    }
    return ptr;

}

uint64_t
L2EncodedValueListNode:: getvalcnt() {
    return lines.size();
}

uint64_t
L2ShortValueListNode::getvalcnt() {
    return uint64list.size() * IOLengthInBytes;
}

double L2ShortValueListNode::expectedOnes(double &prb) {
    map <int, double> tmap;
    L2Node::oth->getrates(tmap);
    prb = 0;
    double ans = 0;
    for (unsigned int i = 1 ; i < uint64list.size(); i++) {
        ans += tmap[i]*valuecnt;
        prb += tmap[i];
    }
    return ans;
}

double L2EncodedValueListNode::expectedOnes(double &prb) {
    map<int,double> tmap;
    L2Node::oth->getrates(tmap);
    vector<uint8_t> cnt8(256);
    for (int t = 0 ; t < 256; t++) {
        int tt = t;
        while (tt) {
            cnt8[t]+=(tt&1);
            tt>>=1;
        }
    }
    double ans = 0;
    prb = 0.0;
    for (unsigned int index = 1; index< lines.size()/IOLengthInBytes; index++) {
        prb += tmap[index];
        if (encodetype == L2NodeTypes::VALUE_INDEX_ENCODED) {
            vector<uint32_t> decode;
            valuelistDecode(&lines[IOLengthInBytes*index], decode, IOLengthInBytes);
            ans += decode.size()  * tmap[index];
        }
        else {
            int tot = 0;
            for (unsigned int j = index*IOLengthInBytes; j < IOLengthInBytes*(index+1); j++)
                tot += cnt8[lines[j]];
            ans += tot * tmap[index];
        }
    }
    return ans;
}
