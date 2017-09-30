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
#include <set>
#include <tinyxml2.h>

using namespace std;
class SeqOthello {

    typedef uint64_t keyType;
    typedef uint16_t freqOthValueT;
public:
    uint32_t kmerLength = 0;
    Othello<keyType> * freqOth = NULL;
    vector<std::shared_ptr<L2Node>> vNodes;
    uint32_t L2IDShift;
	uint32_t sampleCount;
private:
    uint32_t inQlimit = 1048576*512;
    constexpr static uint32_t L2limit = 1048576*32;
    vector<uint32_t> freqToVnodeIdMap;
public:
    SeqOthello() {}

    void loadL2NodeBatch(int thid, string fname, int nthread) {
        printf("Starting to load L2 nodes of grop %d/%d from disk\n", thid, nthread);
        for (int i = 0; i < vNodes.size(); i++)
            if ( i % nthread == thid)
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
    void startloadL2(int nthread) {
        for (int thid = 0; thid < nthread; thid++)
            L2LoadThreads.push_back(
                new thread(&SeqOthello::loadL2NodeBatch, this, thid,fname, nthread));
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
    SeqOthello(string &_fname, int nthread, bool loadall = true) {
        fname = _fname;
        tinyxml2::XMLDocument xml;
        auto xmlName = fname + ".xml";
        xml.LoadFile(xmlName.c_str());
        auto pSeq = xml.FirstChildElement("Root")->FirstChildElement("SeqOthello");
        auto pL2Nodes = pSeq->FirstChildElement("L2Nodes");
        vNodes.clear();
        auto L2Node = pL2Nodes->FirstChildElement("L2Node");
        while (L2Node != NULL) {
            vNodes.push_back(L2Node::loadL2Node(L2Node));
            L2Node = L2Node->NextSiblingElement("L2Node");
        }
        pSeq->QueryIntAttribute("SampleCount", (int*) &sampleCount);
        pSeq->QueryIntAttribute("KmerLength", (int*) &kmerLength);
        pSeq->QueryIntAttribute("L2IDShift", (int*) &L2IDShift);

        /*
        FILE *fin = fopen(fname.c_str(), "rb");
        unsigned char buf[0x20];
        memset(buf,0,sizeof(buf));
        fread(buf,1,sizeof(buf),fin);
        memcpy(&high,buf,4);
        memcpy(&EXP,buf+0x4,4);
        memcpy(&splitbitFreqOth, buf+0x8,4);
        memcpy(&kmerLength, buf+0xC,4);
        memcpy(&L2IDShift, buf+0x10,4);
        fclose(fin);
*/
        if (loadall) {
            startloadL1();
            startloadL2(nthread);
            waitloadL1();
            waitloadL2();
        }
    }

    bool smartQuery(keyType *k, vector<uint32_t> &ret, vector<uint8_t> &retmap) {
        uint64_t othquery = freqOth->queryInt(*k);
        // value : 1..high+1 :  ID = tau - 1
        // high+2 .. : stored in vNode[tau - high - 1]...
        ret.clear();
        if (othquery ==0 ) return true;
        if (othquery < L2IDShift) {
            ret.push_back(othquery-1);
            return true;
        }
        if (othquery - L2IDShift >= vNodes.size()) return true;
        return vNodes[othquery - L2IDShift]->smartQuery(k, ret, retmap);
    }

    void writeSeqOthelloInfo(string fname) {
        tinyxml2::XMLDocument xml;
        auto pRoot = xml.NewElement("Root");
        auto pSeqOthello = xml.NewElement("SeqOthello");
        pSeqOthello->SetAttribute("KmerLength", kmerLength);
        pSeqOthello->SetAttribute("L2IDShift", L2IDShift);
        pSeqOthello->SetAttribute("SampleCount", sampleCount);
        auto pL2Nodes = xml.NewElement("L2Nodes");
        pL2Nodes->SetAttribute("Count", (int) vNodes.size());
        for (auto &p : vNodes) {
            auto pL2Node = xml.NewElement("L2Node");
            p->putInfoToXml(pL2Node);
            pL2Nodes->InsertEndChild(pL2Node);
        }
        pSeqOthello->InsertEndChild(pL2Nodes);
        pRoot->InsertFirstChild(pSeqOthello);
        xml.InsertFirstChild(pRoot);
        auto xmlName = fname + ".xml";
        xml.SaveFile(xmlName.c_str());
    }

    void writeLayer1ToFileAndReleaseMemory(string fname) {
        printf("Writing L1 Node to file %s\n", fname.c_str());
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
        vNodes[id]->constructOth();
        vNodes[id]->writeDataToGzipFile();
    }

    void loadL2Node(int id, string fname) {
        stringstream ss;
        ss<<fname;
        ss<<".L2."<<id;
        string fname2;
        ss >> fname2;
        vNodes[id]->loadDataFromGzipFile();
    }


    void constructFromReader(KmerGroupComposer<keyType> *reader, string filename, uint32_t threadsLimit, vector<uint32_t> enclGrpmap) {
        kmerLength = reader->getKmerLength();
        keyType k;

        printf("We will use at most %d threads to construct.\n", threadsLimit);
        vector<keyType> kV;
        vector<uint16_t> vV;
        vector<uint32_t> ret;
        int maxnl = 1;
        int high = reader->gethigh();
        while ((1<<maxnl)<high) maxnl++;
        uint32_t limitsingle = 64/maxnl;
        L2IDShift = high+2;
		sampleCount = high;
        vector<uint32_t> valshortIDmap(limitsingle+1);
        vector<uint32_t> valshortcnt(limitsingle+1);
        vector<uint32_t> enclGrpIDmap = vector<uint32_t> (1 + *max_element(enclGrpmap.begin(), enclGrpmap.end()));
        vector<uint32_t> enclGrpcnt;// = enclGrpmap;
        vector<uint32_t> enclGrplen;// = enclGrpmap;
        kV.reserve(10485760);
        vV.reserve(10485760);
        enclGrpcnt.resize(enclGrpmap.size());
        enclGrplen.resize(enclGrpmap.size());
        fill(enclGrpcnt.begin(), enclGrpcnt.end(), 0);
        for (unsigned int i = 0 ; i < enclGrpmap.size(); i++)
            enclGrplen[enclGrpmap[i]] = i;


        for (unsigned int i = 2; i<=limitsingle; i++) {
            vNodes.push_back(std::make_shared<L2ShortValueListNode>(i, maxnl));
            valshortIDmap[i] = vNodes.size()-1;
        }
        for (unsigned int i = 0 ; i < enclGrpIDmap.size(); i++) {
            vNodes.push_back(std::make_shared<L2EncodedValueListNode>(enclGrplen[i], L2NodeTypes::VALUE_INDEX_ENCODED));
            enclGrpIDmap[i] = vNodes.size()-1;
        }

        uint32_t MAPPlength = high/8;
        if (high &7) MAPPlength++;

        vNodes.push_back(std::make_shared<L2EncodedValueListNode>(MAPPlength,L2NodeTypes::MAPP));
        uint32_t MAPPID = vNodes.size()-1;
        uint32_t MAPPcnt = 0;

        //value : 1..realhigh+1 :  ID = tau - 1

        // high+2 .. : stored in vNode[tau - realhigh - 1]... -->real high= high <<EXP when exp>1.
        while (reader->getNextValueList(k, ret)) { //now we are getting a pair<id,expression>
            uint32_t valcnt = ret.size();
            kV.push_back(k);

            //cnt = 1
            if (valcnt == 1) {
                vV.push_back(ret[0]+1);
                continue;
            }

            //cnt = 2 ~ limit (encode within 64)
            if (valcnt <= limitsingle) {
                if (valshortcnt[valcnt] * valcnt < L2limit) {
                    valshortcnt[valcnt]++;
                } else {
                    valshortcnt[valcnt] = 0;
                    vNodes.push_back(std::make_shared<L2ShortValueListNode>(valcnt, maxnl));
                    valshortIDmap[valcnt] = vNodes.size() - 1;
                }
                vNodes[valshortIDmap[valcnt]]->add(k, ret);
                vV.push_back(valshortIDmap[valcnt]+ L2IDShift);
                continue;
            }

            vector<uint32_t> diff;
            diff.push_back(ret[0]);
            for (uint32_t i = 1; i < ret.size(); i++)
                diff.push_back(ret[i] - ret[i-1]);
            uint32_t encodelength = valuelistEncode(NULL, diff, false);
            // encode < mapp
            if (encodelength < MAPPlength) {
                vector<uint8_t> enc(encodelength);
                valuelistEncode(&enc[0], diff, true);
                auto grpid = enclGrpmap[encodelength];

                if (enclGrpcnt[grpid] * enclGrplen[grpid] < L2limit) {
                    enclGrpcnt[grpid]++;
                } else {
                    enclGrpcnt[grpid] = 0;
                    vNodes.push_back(std::make_shared<L2EncodedValueListNode>(enclGrplen[grpid], L2NodeTypes::VALUE_INDEX_ENCODED));
                    enclGrpIDmap[grpid] = vNodes.size() - 1;
                }
                vNodes[enclGrpIDmap[grpid]]->add(k, diff);
                vV.push_back(enclGrpIDmap[grpid] + L2IDShift);
                continue;
            }
            if (MAPPcnt * MAPPlength > L2limit)  {
                MAPPcnt = 0;
                vNodes.push_back(std::make_shared<L2EncodedValueListNode>(MAPPlength, L2NodeTypes::MAPP));
                MAPPID = vNodes.size() - 1;
            }
            MAPPcnt++;
            BinaryVarlenBitSet<keyType> kbitmap(k, MAPPlength);
            for (auto const &val: ret) {
                kbitmap.setvalue(val);
            }
            vNodes[MAPPID]->addMAPP(k,kbitmap.m);
            vV.push_back(MAPPID+ L2IDShift);
        }
        for (uint32_t i = 0 ; i < vNodes.size(); i++)
            vNodes[i]->gzfname = filename+"L2."+to_string(i);
        int LLfreq = 8;
        while ((1<<LLfreq)<vNodes.size()+L2IDShift+5) LLfreq++;
        printf("Constructing L1 Node \n");
        freqOth = new Othello<keyType>(LLfreq, kV, vV, true, 10);

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
        for (int i = vNodes.size()-1; i>=0; i--) {
            if (currentInQ > inQlimit || vthreadL2.size()>=threadsLimit) {
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
    }
public:
    static vector<uint32_t> estimateParameters(KmerGroupComposer<keyType> *reader, int kmerlimit) {
        int maxnl = 1;
        int high = reader->gethigh();
        while ((1<<maxnl)<high) maxnl++;
        uint32_t limitsingle = 64/maxnl;
        uint64_t k = 0;
        uint64_t cnt = 0;
        vector<uint32_t> cnthisto(16);
        vector<uint32_t> enchisto(high);
        vector<uint32_t> ret;
        while (reader->getNextValueList(k, ret) && (kmerlimit--)) {
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

        if (kmerlimit ==0) {
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
            if ((sq+ enchisto[i])*i > L2limit) {
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

