#include <L1Node.hpp>
#include <vector>
#include <string>
#include <thread>
#include <zlib.h>
using namespace std;

L1Node::~L1Node() {
    for (uint32_t i = 0 ; i < othellos.size(); i++) {
        delete othellos[i];
        delete kV[i];
        delete vV[i];
    }
    othellos.clear();
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
    printf("%s : Write to Gzip File %s\n", get_thid().c_str(),fname.c_str());
    char cbuf[0x400];
    memset(cbuf,0,sizeof(cbuf));
    sprintf(cbuf,"%s.L1.p%d",fname.c_str(), id);
    gzFile fout = gzopen(cbuf, "wb");
    unsigned char buf[0x20];
    memset(buf,0,sizeof(buf));
    if (kV[id]->size()) {
        othello->exportInfo(buf);
        gzwrite(fout, buf, sizeof(buf));
        othello->writeDataToGzipFile(fout);
    }
    else
        gzwrite(fout,buf,sizeof(buf));

    gzclose(fout);
    delete othello;
    printf("%s : Consturction finished.\n", get_thid().c_str(), id);
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
        sprintf(cbuf,"%s.L1.p%d",fname.c_str(), i);
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
            sprintf(cbuf,"%s.L1.p%d",fname.c_str(), i);
            pNode->SetAttribute("Filename", cbuf);
            pNode->SetAttribute("KeyCount", (uint32_t) kV[i]->size());
            pe->InsertEndChild(pNode);
        }
}
