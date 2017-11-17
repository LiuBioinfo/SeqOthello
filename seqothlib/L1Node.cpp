// This file is a part of SeqOthello. Please refer to LICENSE.TXT for the LICENSE
#include <L1Node.hpp>
#include <vector>
#include <string>
#include <thread>
#include <zlib.h>
using namespace std;

L1Node::~L1Node() {
    for (uint32_t i = 0 ; i < othellos.size(); i++) {
        delete othellos[i];
    }
    othellos.clear();
    for (uint32_t i = 0 ; i <kV.size(); i++) {
        delete kV[i];
        delete vV[i];
    }
    kV.clear();
    vV.clear();
}

void L1Node::add(uint64_t &k, uint16_t v) {
    uint32_t grp = (k)>>(shift);
    if (grp >= grpidlimit)
        throw std::invalid_argument("invalid key to put in L1");
    kV[grp]->push_back(k);
    vV[grp]->push_back(v);
}

uint64_t L1Node::queryInt(uint64_t k) {
    uint32_t grp = (k)>>(shift);
    if (othellos[grp] == NULL)
        return 0;
    return othellos[grp]->queryInt(k);
}

void L1Node::constructothello(uint32_t id, uint32_t L, string fname) {
    Othello<uint64_t> * othello = NULL;
    printf("%s : start to construct L1 Node part %u\n", get_thid().c_str(), id);
    if (kV[id]->size())
        othello = new Othello<uint64_t>(L, *kV[id], *vV[id], true, 200);
    printf("%s : Write to Gzip File %s.%d\n", get_thid().c_str(),fname.c_str(),id);
    char cbuf[0x400];
    memset(cbuf,0,sizeof(cbuf));
    sprintf(cbuf,"%s.%d",fname.c_str(), id);
    gzFile fout = gzopen(cbuf, "wb");
    unsigned char buf[0x20];
    memset(buf,0,sizeof(buf));
    if (kV[id]->size()) {
        othello->exportInfo(buf);
        gzwrite(fout, buf, sizeof(buf));
        othello->writeDataToGzipFile(fout);
        kV[id]->release();
        vV[id]->release();
    }
    else
        gzwrite(fout,buf,sizeof(buf));

    gzclose(fout);
    delete othello;
    printf("%s : L1 part %u consturction finished.\n", get_thid().c_str(), id);
}
void L1Node::constructAndWrite(uint32_t L, uint32_t threads, string fname) {
    vector<thread> vthreadL1;
    uint64_t curreintInQ = 0;

    for (uint32_t i = 0 ; i < grpidlimit; i++) {
        if (curreintInQ  > L1InQlimit || vthreadL1.size()>= threads) {
            for (auto &th : vthreadL1)
                th.join();
            vthreadL1.clear();
            curreintInQ = 0;
        }
        curreintInQ += kV[i]->size();
        vthreadL1.push_back(std::thread(&L1Node::constructothello, this, i, L, fname));
    }
    for (auto &th : vthreadL1)
        th.join();
}
void L1Node::loadFromFile(string fname) {
    grpidlimit = (1<<splitbit);
    othellos.resize(grpidlimit);
    for (uint32_t i = 0 ; i < grpidlimit; i++) {
        char cbuf[0x400];
        memset(cbuf,0,sizeof(cbuf));
        sprintf(cbuf,"%s.%d",fname.c_str(), i);
        gzFile fin = gzopen(cbuf, "rb");
        unsigned char buf[0x20];
        gzread(fin, buf,sizeof(buf));
        unsigned char buf0[0x20];
        memset(buf0,0,sizeof(buf0));
        if (memcmp(buf, buf0, 0x20) ==0) {
            othellos[i] = NULL;
        }
        else {
            othellos[i] = new Othello<uint64_t> (buf);
            othellos[i]->loadDataFromGzipFile(fin);
            if (!othellos[i]->loaded) {
                delete othellos[i];
                othellos[i] = NULL;
            }
        }
        gzclose(fin);
    }
}

void L1Node::putInfoToXml(tinyxml2::XMLElement *pe, string fname) {
    for (unsigned int i = 0 ; i < grpidlimit; i++)
        if (kV[i]->size()) {
            auto pNode = pe->GetDocument()->NewElement("L1NodePart");
            char cbuf[0x400];
            memset(cbuf,0,sizeof(cbuf));
            sprintf(cbuf,"%s.%d",fname.c_str(), i);
            pNode->SetAttribute("Filename", cbuf);
            pNode->SetAttribute("KeyCount", (uint32_t) kV[i]->size());
            pe->InsertEndChild(pNode);
        }
}

map<int,double> L1Node::printrates() {
    map<int, double> sum;
    for (auto *p: othellos) {
        map<int,double> tmap;
        p->getrates(tmap);
        for (auto &x: tmap)
            sum[x.first] += x.second;
    }
    return sum;
}

void L1Node::setfname(string str) {
    fname = str;
}

void L1Node::queryPartAndPutToVV(vector<vector<uint16_t>> &ans, vector<vector<uint64_t>> &kmers, int grp, int threads) {
    if (grp <0 || grp >= (1<<splitbit))
        throw std::invalid_argument("Error group id for L1");

    char cbuf[0x400];
    memset(cbuf,0,sizeof(cbuf));
    sprintf(cbuf,"%s.%d",fname.c_str(), grp);
    gzFile fin = gzopen(cbuf, "rb");
    unsigned char buf[0x20];
    gzread(fin, buf,sizeof(buf));
    unsigned char buf0[0x20];
    memset(buf0,0,sizeof(buf0));
    Othello<uint64_t> *oth;
    if (memcmp(buf, buf0, 0x20) ==0) {
        return;
    }
    else {
        oth = new Othello<uint64_t> (buf);
        oth->loadDataFromGzipFile(fin);
        if (!oth->loaded) {
            delete oth;
            return;
        }
    }
    int maxs = 64;
    vector<int> loc;
    for (int i = 0 ; i<=maxs; i++)
        loc.push_back(kmers.size()*i/maxs);
    for (int thd = 0; thd < 64; thd++)  {
        int st = loc[thd];
        int ed = loc[thd+1];
        for (int i = st ; i < ed; i++)
            for (int j = 0 ; j < kmers[i].size(); j++)
                if (grp == (kmers[i][j] >> shift))
                    ans[i][j] = oth->queryInt(kmers[i][j]);
    }

    delete oth;
    return;
}
