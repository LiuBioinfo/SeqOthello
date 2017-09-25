#pragma once
#include <unordered_map>
#include <thread>
#include <vector>
#include "othello.h"
#define GITVERSION "none"
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <othellotypes.hpp>
#include <string>
#include <L2Node.hpp>


using namespace std;
/*
class vNodeNew {
typedef uint64_t keyType;
typedef uint16_t freqOthValueT;

public:
    uint32_t high, valuecnt, type, EXP, maxnl, IOLengthInBytes, mask, keycnt;
private:
    vector<uint32_t> values;
    vector<uint64_t> keys;
    map<uint64_t, uint32_t> valuemap;
    Othello<keyType> *oth;
    void definetypes() {
        while ((high*EXP) % 8) high ++;
        maxnl = 8;
        while ((1<<maxnl) < high) maxnl++;
        if (EXP>1)
            maxnl += EXP;
        if (maxnl * valuecnt < high*EXP) {
            IOLengthInBytes = maxnl*valuecnt /8;
            while (IOLengthInBytes *8 < maxnl*valuecnt)
                IOLengthInBytes++;
            if (IOLengthInBytes <= 8)
                type = VALUE_INDEX_SHORT;
            else
                type = VALUE_INDEX_LONG; // Currently we are just using vector<uint16_t> for that.
        }
        else {
            type = MAPP;
            IOLengthInBytes = high * EXP;
            while (IOLengthInBytes %8)
                IOLengthInBytes++;
            IOLengthInBytes/=8;
        }
        mask = (1<<maxnl);
        mask--;
    }
    vector<uint64_t> valuelistlist_short;
    vector<uint16_t> valuelistlist;
    vector<uint8_t> lines;
public:
    vNodeNew(uint32_t _high, uint32_t  _valuecnt, uint32_t _EXP) { //each ID is maxnl bits. we suppose to take <= valuecnt IDs.
        high = _high;
        valuecnt = _valuecnt;
        EXP = _EXP;
        keycnt = 0;
        definetypes();
        oth = NULL;
    }
    vNodeNew() {}
    ~vNodeNew() {
        delete oth;
    }

    bool smartQuery(const keyType *k, vector<uint16_t> &ret, vector<uint8_t> &retmap) {
        //when return value is true,
        //  Returns ret: vector of uint16_t,
        //  each element is :
        //     when EXP  ==1 , ID of sample
        //     when EXP >=2, (ID<<EXP)| expression value.
        //when return value is false
        //  Returns retmap
        //  a vector<uint8_t> of exactly IOLengthInBytes bytes.

        uint64_t index = oth->queryInt(*k);
        vector<uint32_t> pq;
        if (type == VALUE_INDEX_SHORT) {
        }
        if (type == VALUE_INDEX_LONG) {
            ret.clear();
            if (index*valuecnt >= valuelistlist.size()) return true;
            for (uint32_t id = index*valuecnt; id < (index+1)*valuecnt; id++)
                if (valuelistlist[id])
                    ret.push_back(valuelistlist[id]);
            return true;
        }
        if (type == MAPP) {
            //TODO : we now only support EXP = 1/2/4
            if ((index+1) * IOLengthInBytes > lines.size()) return true;
            vector<unsigned char> v(lines.begin() + IOLengthInBytes*index, lines.begin() + IOLengthInBytes*(index+1));
            retmap = v;
            return false;
        }
    }

    void
    add(keyType &k, vector<uint16_t> & valuelist) {
        keycnt++;
        if (type == VALUE_INDEX_SHORT) {
        }

        if (type == MAPP) {
            //construct valueline
            uint32_t expmesh = ((1<<EXP)-1);
            BinaryVarlenBitSet<keyType> kbitmap(k, IOLengthInBytes);
            for (auto const val: valuelist) {
                if (EXP>1) {
                    kbitmap.setvalue(val >>EXP, val & expmesh, EXP);
                }
                //TODO : we now only support EXP=1/2/4
                if (EXP == 1) {
                    kbitmap.setvalue(val);
                }
            }
            keys.push_back(k);
            //we always prepend one to avoid \tau result = 0;
            if (lines.size() == 0)
                lines.insert(lines.end(), kbitmap.m.begin(), kbitmap.m.end()); //kbitmap.m should be exactly IOLengthInBytes.
            values.push_back(lines.size()/IOLengthInBytes);
            lines.insert(lines.end(), kbitmap.m.begin(), kbitmap.m.end()); //kbitmap.m should be exactly IOLengthInBytes.
        }
        if (type == VALUE_INDEX_LONG) {
            keys.push_back(k);
            vector<uint16_t> v16;
            v16.reserve(valuecnt);
            for (int i = 0 ; i < valuelist.size(); i++)
                v16.push_back(valuelist[i]);
            while (v16.size() < valuecnt) v16.push_back(0);
            //we always prepend one to avoid \tau result = 0;
            if (valuelistlist.size() ==0)  {
                for (auto const &x: v16)
                    valuelistlist.push_back(x);
            }
            values.push_back(valuelistlist.size()/valuecnt);
            valuelistlist.insert(valuelistlist.end(), v16.begin(), v16.end());
        }
    }
    void addMAPP(keyType &k, vector<uint8_t> &mapp) {
        if (lines.size() ==0)
            lines = mapp;
        keys.push_back(k);
        values.push_back(lines.size()/IOLengthInBytes);
        lines.insert(lines.end(), mapp.begin(), mapp.end());
    }
    void construct() {
        int L = 8;
        //valuelistlist_short
        //valuelistlist
        //lines
        int p1 = (valuelistlist_short.size())?1:0;
        int p2 = (valuelistlist.size())?1:0;
        int p3 = (lines.size())?1:0;
        if (p1+p2+p3 > 1) {
            cerr <<" Error, must either be one of :MAPP, Index, or Index_long"<< endl;
            cerr <<"vll_s"<<valuelistlist_short.size()<<" vll"<<valuelistlist.size()<<"  lines"<< lines.size() <<endl;
            return;

        }
        int possibleMaxValue = (valuelistlist_short.size()) + (valuelistlist.size()) + (lines.size() / (IOLengthInBytes));
        while ((1<<L)<possibleMaxValue) L++;

        printf("construct L2Node with %lld keys. Using type %s. with %d-Othello, valuecnt %d.\n", keys.size(), typestr.at(type).c_str(), L, valuecnt);
        oth = new Othello<keyType> (L, keys,values, true, 0);

        for (auto &k: oth->removedKeys) {
            printf("Removed key vNode %llx\n", k);
        }
    }

    void writeDataToGzipFile(gzFile fout) {
        unsigned char buf[0x20];
        memset(buf,0,sizeof(buf));

        uint32_t linescount = lines.size();
        memcpy(buf, &high, 4);
        memcpy(buf+4,&valuecnt, 4);
        memcpy(buf+8,&EXP, 4);
        uint32_t Mappsize = lines.size();
        memcpy(buf+0xC, &Mappsize, 4);
        uint32_t VLcount = valuelistlist.size();
        memcpy(buf+0x10, &VLcount, 4);
        uint32_t ShortVLcount = valuelistlist_short.size();
        memcpy(buf+0x14, &ShortVLcount, 4);

        gzwrite(fout, buf,sizeof(buf));
        oth->exportInfo(buf);
        gzwrite(fout, buf,sizeof(buf));
        oth->writeDataToGzipFile(fout);
        if (type == MAPP) {
            gzwrite(fout, &lines[0], Mappsize* sizeof(uint8_t));
            printf("%s, %d bytes for valuecnt %d, IOL %d.\n", typestr.at(type).c_str(), Mappsize, valuecnt, IOLengthInBytes);
        }
        if (type == VALUE_INDEX_SHORT) {
            for (auto const & vl: valuelistlist_short) {
                uint64_t rvl = vl;
                gzwrite(fout, &rvl, IOLengthInBytes);
            }
            printf("%s, %d bytes for valuecnt %d.\n", typestr.at(type).c_str(), IOLengthInBytes* valuelistlist_short.size(), valuecnt);
        }

        if (type == VALUE_INDEX_LONG) {
            gzwrite(fout, &valuelistlist[0], valuelistlist.size()* sizeof(valuelistlist[0]));
        }
    }

    void loadDataFromGzipFile(gzFile fin) {
        unsigned char buf[0x20];
        gzread(fin, buf,sizeof(buf));
        void *p;
        p = &buf;
        memcpy(&high, p, 4);
        memcpy(&valuecnt, p+4, 4);
        memcpy(&EXP, p+0x8, 4);
        uint32_t Mappsize, VLcount, ShortVLcount;
        memcpy(&Mappsize, p+0xC, 4);
        memcpy(&VLcount, p+0x10, 4);
        memcpy(&ShortVLcount, p+0x14, 4);
        definetypes();
        gzread(fin, buf,sizeof(buf));
        oth = new Othello<uint64_t> (buf);
        oth->loadDataFromGzipFile(fin);

        if (type == MAPP) {
            lines.resize(Mappsize);
            gzread(fin, &lines[0], Mappsize* sizeof(uint8_t));
        }
        if (type == VALUE_INDEX_SHORT) {
            valuelistlist_short.resize(0);//ShortVLcount);
            for (int i = 0 ; i < ShortVLcount; i++) {
                uint64_t vl = 0ULL;
                gzread(fin, &vl, IOLengthInBytes);
                valuelistlist_short.push_back(vl);
            }
        }
        if (type == VALUE_INDEX_LONG) {
            valuelistlist.resize(VLcount);
            gzread(fin, &valuelistlist[0], VLcount* sizeof(valuelistlist[0]));
        }
    }
};

*/
class SeqOthello {
        /*
typedef uint64_t keyType;
typedef uint16_t freqOthValueT;
public:
    uint32_t high, EXP, splitbitFreqOth, kmerLength;
    Othello<keyType> * freqOth = NULL;
    vector<std::shared_ptr<vNode>> vNodes;
    uint32_t realhigh;
private:
    uint32_t concurentThreadsSupported;
    uint32_t inQlimit = 1048576*1024;
    vector<uint32_t> freqToVnodeIdMap;
    void preparevNodes() {
        realhigh = high;
        if (EXP>1) realhigh <<= EXP;
        concurentThreadsSupported = std::thread::hardware_concurrency();
        if (concurentThreadsSupported == 0)
            concurentThreadsSupported = 2;
        vNodes.clear();
        if (concurentThreadsSupported <4) concurentThreadsSupported = 4;
        freqToVnodeIdMap= vector<uint32_t>(high+2, 0);
        vNodes.push_back(nullptr);
        //These values are designed for the 2562 human data!
        //
        for (int i = 1; i< 165 && i < high; i++) {
            vNodes.push_back(std::make_shared<vNodeNew>(high, i, EXP));
        }
        for (int i = 165; i< 1600 && i < high; i+=5)
            vNodes.push_back(std::make_shared<vNodeNew>(high, i, EXP));
        vNodes.push_back(std::make_shared<vNodeNew>(high, high, EXP));
        for (int i = 1; i<= high; i++) {
            int j = 1;
            while (vNodes[j]->valuecnt < i) j++;
            freqToVnodeIdMap[i] = j ;
        }
    }

public:
    SeqOthello() {}

    void loadL2NodeBatch(int thid, string fname) {
        printf("Starting to load L2 nodes of grop %d/%d from disk\n", thid, concurentThreadsSupported);
        for (int i = 1; i < vNodes.size(); i++)
            if ( i % concurentThreadsSupported == thid)
                loadL2Node(i, fname);
    }
    thread * L1LoadThread;
    vector<thread *> L2LoadThreads;
    string fname;
    void startloadL1() {
        printf("Starting to load L1 from disk\n");
        string ff = fname + ".L1";
        gzFile fin = gzopen(ff.c_str(), "rb");
        unsigned char buf[0x20];
        gzread(fin, buf,sizeof(buf));
        freqOth = new Othello<uint64_t> (buf);
        L1LoadThread = new std::thread (
                &Othello<uint64_t>::loadDataFromGzipFile,
                freqOth,
                fin);

    }
    void startloadL2() {
        for (int thid = 0; thid < concurentThreadsSupported; thid++)
            L2LoadThreads.push_back(
                    new thread(&SeqOthello::loadL2NodeBatch, this, thid,fname));
    }
    void waitloadL2() {
            for (auto p : L2LoadThreads)
                p->join();
            L2LoadThreads.clear();
        printf("Load L2 finished \n");
    }
    void waitloadL1() {
        L1LoadThread->join();
        delete L1LoadThread;
        printf("Load L1 finished \n");
    }
    void releaseL1() {
        delete freqOth;
    }
    SeqOthello(string &_fname, bool loadall = true) {
        fname = _fname;
        FILE *fin = fopen(fname.c_str(), "rb");
        unsigned char buf[0x20];
        memset(buf,0,sizeof(buf));
        fread(buf,1,sizeof(buf),fin);
        memcpy(&high,buf,4);
        memcpy(&EXP,buf+0x4,4);
        memcpy(&splitbitFreqOth, buf+0x8,4);
        memcpy(&kmerLength, buf+0xC,4);
        preparevNodes();
        fclose(fin);

        if (loadall) {
            startloadL1();
            startloadL2();
            waitloadL1();
            waitloadL2();
        }
    }
    bool smartQuery(keyType *k, uint16_t othquery, vector<uint16_t> &ret, vector<uint8_t> &retmap) {
        // value : 1..high+1 :  ID = tau - 1
        // high+2 .. : stored in vNode[tau - high - 1]...
        ret.clear();
        if (othquery ==0 ) return true;
        if (othquery <= realhigh + 1) {
            ret.push_back(othquery-1);
            return true;
        }
        if (othquery - realhigh - 1 >= vNodes.size()) return true;
        return vNodes[othquery - realhigh - 1]->smartQuery(k, ret, retmap);
    }

    bool smartQuery(keyType *k, vector<uint16_t> &ret, vector<uint8_t> &retmap) {
        uint64_t othquery = freqOth->queryInt(*k);
        return smartQuery(k, othquery, ret, retmap);

    }

    void writeSeqOthelloInfo(string fname) {
        FILE *fout = fopen(fname.c_str(), "wb");
        unsigned char buf[0x20];
        memset(buf,0,sizeof(buf));
        memcpy(buf,&high,4);
        memcpy(buf+0x4,&EXP,4);
        memcpy(buf+0x8,&splitbitFreqOth,4);
        memcpy(buf+0xC,&kmerLength,4);
        fwrite(buf,1,sizeof(buf),fout);
    }

    void writeLayer1ToFileAndReleaseMemory(string fname) {
        printf("Writing L1 Node to file\n");
        writeSeqOthelloInfo(fname);

        fname += ".L1";
        gzFile fout = gzopen(fname.c_str(), "wb");
        unsigned char buf[0x20];
        freqOth->exportInfo(buf);
        gzwrite(fout, buf,sizeof(buf));
        freqOth->writeDataToGzipFile(fout);

        gzclose(fout);
        delete freqOth;
        inQlimit *= 2;
    }

    void constructL2Node(int id, string fname) {
        printf("constructing L2 Node %d\n", id);
        stringstream ss;
        ss<<fname;
        ss<<".L2."<<id;
        string fname2;
        ss >> fname2;
        gzFile fout = gzopen(fname2.c_str(), "wb");
        vNodes[id]->construct();
        vNodes[id]->writeDataToGzipFile(fout);
        gzclose(fout);
    }

    void loadL2Node(int id, string fname) {
        stringstream ss;
        ss<<fname;
        ss<<".L2."<<id;
        string fname2;
        ss >> fname2;
        gzFile fin = gzopen(fname2.c_str(), "rb");
        vNodes[id]->loadDataFromGzipFile(fin);
        gzclose(fin);
    }
    */

    /*
    void constructFromReader(GrpReader<keyType> *reader, string filename, uint32_t allocate=1048576*32) {
        keyType k;
        vector< pair<uint32_t, uint32_t> > ret;
        high = reader->gethigh();
        preparevNodes();
        printf("We will use at most %d threads to construct. At most %d keys(valuelists) at the same time.\n", concurentThreadsSupported, inQlimit);
        vector<keyType> kV;
        vector<uint16_t> vV;
        kV.reserve(allocate);
        vV.reserve(allocate);
        //value : 1..realhigh+1 :  ID = tau - 1
        // high+2 .. : stored in vNode[tau - realhigh - 1]... -->real high= high <<EXP when exp>1.
        while (reader->getNextValueList(k, ret)) { //now we are getting a pair<id,expression>
            uint32_t valcnt = ret.size();
            kV.push_back(k);
            if (valcnt == 1) {
                vV.push_back((ret.begin()->first)+1);
                continue;
            }
            vector<uint16_t> vallist;
            for (auto const &p: ret) {
                uint16_t id = p.first;
                if (EXP>1) {
                    id <<=EXP;
                    id |= p.second;
                }
                vallist.push_back(id);
            }
            uint32_t nodeid = freqToVnodeIdMap[vallist.size()];
            vNodes[nodeid]->add(k, vallist);
            vV.push_back(nodeid + realhigh + 1);
        }
        int LLfreq = 8;
        while ((1<<LLfreq)<realhigh) LLfreq++;
        printf("Constructing L1 Node \n");
        freqOth = new Othello<keyType>(LLfreq, kV, vV, true, 0);
        kV.clear();
        vV.clear();
        vector<thread> vthreadL1;
        vector<thread> vthreadL2;
        for (int i = 0 ; i < kV.size(); i++) {
            uint32_t ret = freqOth->queryInt(kV[i]);
            if (ret != vV[i]) {
                printf("%!!!!!!!!!!%llx %d %d\n",kV[i], vV[i], ret);
            }
        }
        vthreadL1.push_back(std::thread(&SeqOthello::writeLayer1ToFileAndReleaseMemory,this,filename));
        uint32_t currentInQ = 0;
        for (int i = vNodes.size()-1; i>=1; i--) {
            if (currentInQ > inQlimit || vthreadL2.size()>=concurentThreadsSupported) {
                for (auto &th : vthreadL2) th.join();
                vthreadL2.clear();
                currentInQ = 0;
            }
            currentInQ += vNodes[i]->keycnt;
            vthreadL2.push_back(std::thread(&SeqOthello::constructL2Node,this,i, filename));
        }

        for (auto &th : vthreadL2) th.join();
        for (auto &th : vthreadL1) th.join();
        vthreadL2.clear();
        SeqOthello<keyType> cmp(filename);
        printf("here to compare\n");
    }
	*/
public:            
    static vector<uint32_t> estimateParameters(KmerGroupComposer<keyType> *reader) {
        int maxnl = 8;
        int high = reader->gethigh();
        while ((1<<maxnl)<high) maxnl++;
        uint32_t limitsingle = 64/maxnl;
        uint64_t k = 0;
        int64_t cnt = 0;
        vector<uint32_t> cnthisto(16);
        vector<uint32_t> enchisto(high);
		vector<uint32_t> ret;
        int limit = ESTIMATE_KMER_CNT;
        while (reader->getNextValueList(k, ret) && (limit--)) {
            cnt ++;
            int keycnt = ret.size();
            if (keycnt <= limitsingle) {
                    cnthisto[keycnt]++;
            } 
            else {
                vector<uint32_t> toenc;
                toenc.reserve(ret.size());
                toenc.push_back(ret[0]+1);
                for (int i = 1; i< ret.size(); i++)
                    toenc.push_back(ret[i] - ret[i-1]);
                int encodelength = valuelistEncode(NULL, toenc, false);
                if (encodelength*8 > high)
                        encodelength = (high/8)+(bool(high&7));
                enchisto[encodelength]++;
            }
        }

        if (limit ==0) {
                vector<uint64_t> curr, tot;
                reader->getGroupStatus(curr, tot);
                uint64_t currA = accumulate(curr.begin(), curr.end(), 0ULL);
                uint64_t totA = accumulate(tot.begin(), tot.end(), 0ULL);
                double rate = totA * 1.0 / currA;
                for (auto &x : enchisto) 
                        x = (uint64_t) (x * rate);
                for (auto &x : cnthisto)
                        x = (uint64_t) (x * rate);
                cnt =  (uint64_t) (cnt * rate);
        }
        vector<uint32_t> encodeLengthToL1ID(high/8+2);
        uint64_t sq = 0;
        uint64_t l1id = 0;
        for (int i = 1; i< high/8+2; i++) {
                printf("%d:%d\n", i, enchisto[i]);
        }
        for (int i = 1 ; i < high/8+2; i++) {
            if ((sq+ enchisto[i])*i > LIMT_PER_L2) {
                sq = 0;
                l1id ++;
            }
            sq += enchisto[i];
            encodeLengthToL1ID[i] = l1id;
        }
        reader->reset();
        return encodeLengthToL1ID;
    }
};

