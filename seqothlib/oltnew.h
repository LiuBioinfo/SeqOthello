// This file is a part of SeqOthello. Please refer to LICENSE.TXT for the LICENSE
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
#include <L1Node.hpp>
#include <functional>
using namespace std;
class SeqOthello {

    typedef uint64_t keyType;
    typedef uint16_t freqOthValueT;
public:
    uint32_t kmerLength = 0;
    L1Node * l1Node = NULL;
    vector<std::shared_ptr<L2Node>> vNodes;
    uint32_t L2IDShift;
    uint32_t sampleCount;
    uint32_t L1Splitbit;
    SeqOthello() {}
private:
#ifndef NDEBUG
    uint32_t L2InQKeyLimit = 1024512;
    uint64_t L2InQValLimit = 1024ULL*32ULL;
    uint32_t L2limit = 128;
    constexpr static uint32_t L2limit0 = 128;
    uint32_t L2diff = L2limit;
#else
    uint32_t L2InQKeyLimit = 1048576*512;
    uint64_t L2InQValLimit = 1048576ULL*1024ULL*32ULL;
    uint32_t L2limit = 1048576*128;
    constexpr static uint32_t L2limit0 = 1048576*128;
    uint32_t L2diff = L2limit/128;
#endif    
    vector<uint32_t> freqToVnodeIdMap;
    string folder;
    string L1NODE_PREFIX="map.L1.p";
    string L2NODE_PREFIX="map.L2.";
    string XML_FNAME="map.xml";
    vector<bool> needToLoad;

    void loadL2NodeBatch(uint32_t thid, string _folder, uint32_t nthread) {
        folder = _folder;
        printf("%s: Starting to load L2 nodes of grop %d/%d from disk\n", get_thid().c_str(), thid, nthread);
        for (uint32_t i = 0; i < vNodes.size(); i++) {
            if (i<needToLoad.size()) 
                if (!needToLoad[i]) 
                    continue;
            if ( i % nthread == thid)
                loadL2Node(i);
        }
    }
    thread * L1LoadThread;
    vector<thread *> L2LoadThreads;
private:
    void loadL1(uint32_t kmerLength) {
        l1Node = new L1Node();
        l1Node->setsplitbit(kmerLength,L1Splitbit);
        l1Node->loadFromFile(folder + L1NODE_PREFIX);
        /*
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
        */
    }
    void startloadL2(int nthread, vector<bool> _needToLoad = vector<bool>()) {
        needToLoad = _needToLoad;
        for (int thid = 0; thid < nthread; thid++)
            L2LoadThreads.push_back(
                new thread(&SeqOthello::loadL2NodeBatch, this, thid,folder, nthread));
    }
    void waitloadL2() {
        for (auto p : L2LoadThreads)
            p->join();
        L2LoadThreads.clear();
        printf("Load L2 finished \n");
    }
public:
    void releaseL1() {
        delete l1Node;
    }
    void releaseL2Node(int id) {
          vNodes[id].reset(); 
    }
    SeqOthello(string &_folder, int nthread, bool loadall = true) {
        folder = _folder;
        tinyxml2::XMLDocument xml;
        auto xmlName = folder + XML_FNAME;
        xml.LoadFile(xmlName.c_str());
        auto pSeq = xml.FirstChildElement("Root")->FirstChildElement("SeqOthello");
        auto pL2Nodes = pSeq->FirstChildElement("L2Nodes");
        vNodes.clear();
        auto L2Node = pL2Nodes->FirstChildElement("L2Node");
        while (L2Node != NULL) {
            vNodes.push_back(L2Node::createL2Node(L2Node));
            L2Node = L2Node->NextSiblingElement("L2Node");
        }
        pSeq->QueryIntAttribute("SampleCount", (int*) &sampleCount);
        pSeq->QueryIntAttribute("KmerLength", (int*) &kmerLength);
        pSeq->QueryIntAttribute("L2IDShift", (int*) &L2IDShift);
        pSeq->QueryIntAttribute("L1SplitBit", (int*) &L1Splitbit);

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
            loadAll(nthread);
        }
    }

    bool smartQuery(keyType *k, vector<uint32_t> &ret, vector<uint8_t> &retmap) {
        uint64_t othquery = l1Node->queryInt(*k);
        // value : 1..high+1 :  ID = tau - 1
        // high+2 .. : stored in vNode[tau - high - 1]...
        ret.clear();
        if (othquery ==0 ) return true;
        if (othquery < L2IDShift) {
            ret.push_back(othquery-1);
            return true;
        }
        if (othquery - L2IDShift >= vNodes.size()) return true;
        if (!vNodes[othquery-L2IDShift] ) return true;
        return vNodes[othquery - L2IDShift]->smartQuery(k, ret, retmap);
    }

    void writeSeqOthelloInfo(string folder, function<void(tinyxml2::XMLElement *)> func, vector<uint64_t> &histogram) {
        tinyxml2::XMLDocument xml;
        auto pRoot = xml.NewElement("Root");
        auto pSeqOthello = xml.NewElement("SeqOthello");
        pSeqOthello->SetAttribute("KmerLength", kmerLength);
        pSeqOthello->SetAttribute("L2IDShift", L2IDShift);
        pSeqOthello->SetAttribute("SampleCount", sampleCount);
        pSeqOthello->SetAttribute("L1SplitBit", l1Node->getsplitbit());
        auto pL2Nodes = xml.NewElement("L2Nodes");
        auto pL1Node = xml.NewElement("L1Node");
        auto pSamples = xml.NewElement("Samples");
        auto pHistogram = xml.NewElement("Histogram");
        for (unsigned int i = 0 ; i < histogram.size(); i++)
            if (histogram[i]) {
                auto pHisNode = xml.NewElement("entry");
                pHisNode->SetAttribute("freq", i);
                pHisNode->SetAttribute("value", (uint32_t) histogram[i]);
                pHistogram->InsertEndChild(pHisNode);
            }

        func(pSamples);
        l1Node->putInfoToXml(pL1Node, L1NODE_PREFIX );
        pL2Nodes->SetAttribute("Count", (int) vNodes.size());
        for (uint32_t i = 0 ; i < vNodes.size(); i++) {
            auto pL2Node = xml.NewElement("L2Node");
            vNodes[i]->putInfoToXml(pL2Node);
            pL2Nodes->InsertEndChild(pL2Node);
        }
        pSeqOthello->InsertEndChild(pL2Nodes);
        pSeqOthello->InsertEndChild(pL1Node);
        pSeqOthello->InsertFirstChild(pSamples);
        pSeqOthello->InsertEndChild(pHistogram);
        pRoot->InsertFirstChild(pSeqOthello);
        xml.InsertFirstChild(pRoot);
        auto xmlName = folder + XML_FNAME;
        xml.SaveFile(xmlName.c_str());
    }
    string toL2Name(int id) {
        stringstream ss;
        ss<<folder;
        ss<<L2NODE_PREFIX<<id;
        string fname2;
        ss >> fname2;
        return fname2;
    }
    set<int> constructedL2;
    void constructL2Node(int id) {
        if (constructedL2.count(id)) 
                return;
        printf("%s: constructing L2 Node %d\n", get_thid().c_str(), id);
        vNodes[id]->constructOth();
        vNodes[id]->writeDataToGzipFile();
        constructedL2.insert(id);
    }
    void startBuildOneL2(int id) {
        // TODO : start a thread...
        constructL2Node(id);
    }
    void loadL2Node(int id) {
        if (!vNodes[id]) {
            printf("%s : Skipping empty L2Node %d\n", get_thid().c_str(), id);
            return;
        }
        string fname2 = toL2Name(id);
        printf("%s : Loading %s\n", get_thid().c_str(), fname2.c_str());
        vNodes[id]->loadDataFromGzipFile();
        printf("%s : Finished %s\n", get_thid().c_str(), fname2.c_str());
        if (vNodes[id]->oth)
            if (vNodes[id]->oth->loaded)
                return;
        printf("Empty L2 Node %d.\n", id);
        vNodes[id].reset();
    }
    void loadAll(int nloadThreads) {
        loadL1(kmerLength);
        startloadL2(nloadThreads);
        waitloadL2();
    }
    void constructFromReader(KmerGroupComposer<keyType> *reader, string filename, uint32_t threadsLimit, vector<uint32_t> enclGrpmap, uint64_t estimatedKmerCount) {
        kmerLength = reader->getKmerLength();
        folder = filename;
        keyType k;
        l1Node = new L1Node(estimatedKmerCount, kmerLength, filename+"tmp");
        printf("We will use at most %d threads to construct.\n", threadsLimit);
        printf("Use encode length to split L2 nodes at: ");
        for (uint32_t i = 1; i < enclGrpmap.size(); i++) {
            if (enclGrpmap[i] != enclGrpmap[i-1]) {
                printf("%d \t",i);
            }
        }
        printf("\n");
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
        vector<uint64_t> histogram(high+1);
        enclGrpcnt.resize(enclGrpmap.size());
        enclGrplen.resize(enclGrpmap.size());
        fill(enclGrpcnt.begin(), enclGrpcnt.end(), 0);
        for (unsigned int i = 0 ; i < enclGrpmap.size(); i++)
            enclGrplen[enclGrpmap[i]] = i;


        for (unsigned int i = 2; i<=limitsingle; i++) {
            vNodes.push_back(std::make_shared<L2ShortValueListNode>(i, maxnl, toL2Name(vNodes.size())));
            valshortIDmap[i] = vNodes.size()-1;
        }
        for (unsigned int i = 0 ; i < enclGrpIDmap.size(); i++) {
            vNodes.push_back(std::make_shared<L2EncodedValueListNode>(enclGrplen[i], L2NodeTypes::VALUE_INDEX_ENCODED,toL2Name(vNodes.size())));
            enclGrpIDmap[i] = vNodes.size()-1;
        }

        uint32_t MAPPlength = high/8;
        if (high &7) MAPPlength++;

        vNodes.push_back(std::make_shared<L2EncodedValueListNode>(MAPPlength,L2NodeTypes::MAPP, toL2Name(vNodes.size())));
        uint32_t MAPPID = vNodes.size()-1;
        uint32_t MAPPcnt = 0;

        //value : 1..realhigh+1 :  ID = tau - 1

        // high+2 .. : stored in vNode[tau - realhigh - 1]... -->real high= high <<EXP when exp>1.
        while (reader->getNextValueList(k, ret)) { //now we are getting a pair<id,expression>
            uint32_t valcnt = ret.size();
            histogram[valcnt] ++;
            //cnt = 1
            if (valcnt == 1) {
                l1Node->add(k, ret[0]+1);
                //vV.push_back(ret[0]+1);
                continue;
            }

            //cnt = 2 ~ limit (encode within 64)
            if (valcnt <= limitsingle) {
                if (valshortcnt[valcnt] * valcnt < L2limit) {

                    valshortcnt[valcnt]++;
                } else {
                    valshortcnt[valcnt] = 0;
                    startBuildOneL2(valshortIDmap[valcnt]);
                    vNodes.push_back(std::make_shared<L2ShortValueListNode>(valcnt, maxnl, toL2Name(vNodes.size())));
                    L2limit+=(vNodes.size()*L2diff);
                    if (((512 - vNodes.size()) & (511-vNodes.size()))== 0) L2diff*=2;
                    valshortIDmap[valcnt] = vNodes.size() - 1;
                }
                vNodes[valshortIDmap[valcnt]]->add(k, ret);
                //vV.push_back(valshortIDmap[valcnt]+ L2IDShift);
                l1Node->add(k, valshortIDmap[valcnt]+ L2IDShift);
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
                    startBuildOneL2(enclGrpIDmap[grpid]);
                    vNodes.push_back(std::make_shared<L2EncodedValueListNode>(enclGrplen[grpid], L2NodeTypes::VALUE_INDEX_ENCODED, toL2Name(vNodes.size())));
                    L2limit+=(vNodes.size()*L2diff);
                    if (((512 - vNodes.size()) & (511-vNodes.size()))== 0) L2diff*=2;
                    enclGrpIDmap[grpid] = vNodes.size() - 1;
                }
                vNodes[enclGrpIDmap[grpid]]->add(k, diff);
                //vV.push_back(enclGrpIDmap[grpid] + L2IDShift);
                l1Node->add(k, enclGrpIDmap[grpid] + L2IDShift);
                continue;
            }
            if (MAPPcnt * MAPPlength > L2limit)  {
                MAPPcnt = 0;
                startBuildOneL2(MAPPID);
                vNodes.push_back(std::make_shared<L2EncodedValueListNode>(MAPPlength, L2NodeTypes::MAPP, toL2Name(vNodes.size())));
                L2limit+=(vNodes.size()*L2diff);
                if (((512 - vNodes.size()) & (511-vNodes.size()))== 0) L2diff*=2;
                MAPPID = vNodes.size() - 1;
            }
            MAPPcnt++;
            BinaryVarlenBitSet<keyType> kbitmap(k, MAPPlength);
            for (auto const &val: ret) {
                kbitmap.setvalue(val);
            }
            vNodes[MAPPID]->addMAPP(k,kbitmap.m);
            //vV.push_back(MAPPID+ L2IDShift);
            l1Node->add(k, MAPPID+ L2IDShift);
        }
        int LLfreq = 8;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
        while ((1<<LLfreq)<vNodes.size()+L2IDShift+5) LLfreq++;
#pragma GCC diagnostic pop
        printf("Got %lu kmers.\n", reader->keycount);
        printf("Constructing L1 Node \n");
        writeSeqOthelloInfo(folder, bind(&KmerGroupComposer<keyType>::putSampleInfoToXml, reader, placeholders::_1 ), histogram);
        vector<thread> vthreadL2;
        uint64_t currL2InQKeycnt = 0;
        uint64_t currL2InQValcnt = 0;
        for (int i = vNodes.size()-1; i>=0; i--) {
            if ((currL2InQKeycnt> L2InQKeyLimit) || (currL2InQValcnt > L2InQValLimit) || vthreadL2.size()>=threadsLimit) {
                for (auto &th : vthreadL2) th.join();
                vthreadL2.clear();
                currL2InQKeycnt = currL2InQValcnt = 0;
            }
            currL2InQKeycnt+= vNodes[i]->keycnt;
            currL2InQValcnt+= vNodes[i]->getvalcnt();
            vthreadL2.push_back(std::thread(&SeqOthello::constructL2Node,this,i));
        }

        for (auto &th : vthreadL2) th.join();
        vthreadL2.clear();
        l1Node->constructAndWrite(LLfreq, threadsLimit, folder+ L1NODE_PREFIX);
        delete l1Node;
    }
    static vector<uint32_t> estimateParameters(KmerGroupComposer<keyType> *reader, int kmerlimit, uint64_t &estimateKmerCnt) {
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
            uint32_t keycnt = ret.size();
            if (keycnt <= limitsingle) {
                cnthisto[keycnt]++;
            }
            else {
                vector<uint32_t> toenc;
                toenc.reserve(ret.size());
                toenc.push_back(ret[0]+1);
                for (uint32_t i = 1; i< ret.size(); i++)
                    toenc.push_back(ret[i] - ret[i-1]);
                int encodelength = valuelistEncode(NULL, toenc, false);
                if (encodelength*8 > high)
                    encodelength = (high/8)+(bool(high&7));
                enchisto[encodelength]++;
            }
        }
        //printf("%d\n",kmerlimit);
        if (kmerlimit <=0) {
            vector<uint64_t> curr, tot;
            reader->getGroupStatus(curr, tot);
            uint64_t currA = accumulate(curr.begin(), curr.end(), 0ULL);
            uint64_t totA = accumulate(tot.begin(), tot.end(), 0ULL);

            double rate = totA * 1.0 / currA;
            //printf("---> %llf %lld %lld\n", rate, totA, currA);
            for (auto &x : enchisto)
                x = (uint64_t) (x * rate);
            for (auto &x : cnthisto)
                x = (uint64_t) (x * rate);
            estimateKmerCnt = cnt =  (uint64_t) (cnt * rate);
        }
        vector<uint32_t> encodeLengthToL1ID(high/8+2);
        uint64_t sq = 0;
        uint64_t l1id = 0;

        printf("Estimated histogram for encode lengths:");
        for (int i = 1; i< high/8+2; i++) {
            printf("%d:%d\t", i, enchisto[i]);
        }
        printf("\n");

        for (int i = 1 ; i < high/8+2; i++) {
            if ((sq+ enchisto[i])*i > L2limit0) {
                sq = 0;
                l1id ++;
            }
            sq += enchisto[i];
            encodeLengthToL1ID[i] = l1id;
        }
        reader->reset();
        return encodeLengthToL1ID;
    }
    vector<vector<uint16_t>> QueryL1ByPartition(vector<vector<uint64_t>> &kmers, int nThreads) {
        l1Node = new L1Node();
        l1Node->setsplitbit(kmerLength,L1Splitbit);
        l1Node->setfname(folder + L1NODE_PREFIX);
        //loadL1(kmerLength);
        vector<vector<uint16_t>> ans;
        for (auto &vk:kmers)
                ans.push_back(vector<uint16_t>(vk.size()));
        for (int grp = 0; grp < (1<<L1Splitbit); grp++)
                l1Node->queryPartAndPutToVV(ans,kmers,grp,nThreads);
        return ans;
    }
    void printrates() {
        map<int, double> rates = l1Node->printrates();
        for (unsigned int i = 1; i<L2IDShift; i++) 
            printf("%d : %.8lf 1.0 1.0\n", i, rates[i]);
        for (unsigned int i = 0 ; i < vNodes.size(); i++) {
            double t;
            double exp = vNodes[i]->expectedOnes(t);
            printf("%d : %.8lf %.5lf %.3lf\n", i+L2IDShift, rates[i+L2IDShift] ,t, exp);
        }
    }
};


