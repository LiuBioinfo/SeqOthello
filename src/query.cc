#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <bitset>
#include "oltnew.h"
#include <chrono>
#include <inttypes.h>
#include <string>
#include <map>
#include <unordered_map>
#include <args.hxx>
#include <io_helper.hpp>
#include <atomic>
using namespace std;

int nqueryThreads = 1;
atomic<int> workers;
void getL1Result(shared_ptr<SeqOthello>seqoth, const vector<string> & seq, const vector<int> & seqID,
                 vector<uint16_t> &result, vector<uint64_t> &kmers, vector<uint32_t> & TID, bool useReverseComp) {
    int32_t kmerLength = seqoth->kmerLength;
    ConstantLengthKmerHelper<uint64_t, uint16_t> helper(seqoth->kmerLength,0);

    printf("%s : Got %lu transcripts for L1 query. \n", get_thid().c_str(), seq.size());
    result.clear();
    TID.clear();
    kmers.clear();
    for (unsigned int id = 0; id < seq.size(); id++) {
        auto const & str = seq[id];
        int ul = str.size()-kmerLength+1;
        char buf[64];
        memset(buf,0,sizeof(buf));
        if (ul>0)
            for (unsigned int i = 0 ; i < str.size() - kmerLength + 1; i++) {
                memcpy(buf,str.data()+i,kmerLength);
                uint64_t key;
                helper.convert(buf,&key);
                if (useReverseComp) {
                    key =  helper.minSelfAndRevcomp(key);
                }
                kmers.push_back(key);
                result.push_back(seqoth->l1Node->queryInt(key));
                TID.push_back(seqID[id]);
            }
    }
    printf("%s: L1 finished. Got %lu kmers. \n", get_thid().c_str(), TID.size());
};
void getL2Result(int32_t high, vector<shared_ptr<L2Node>> pvNodes, const vector<shared_ptr<vector<uint64_t>>> & vpvkmer, const vector<shared_ptr<vector<uint32_t>>> &vTID, map<int, vector<int>> *pans) {

    int myid = workers.fetch_add(1);
    unsigned int totkmer = 0;
    for (auto const & p:vpvkmer)
        if (p)
            totkmer += p->size();
    printf("%s : Query on %lu L2 nodes, with %u kmers.\n", get_thid().c_str(), pvNodes.size(), totkmer);
    pans->clear();
    for (unsigned int vpid = 0; vpid < vpvkmer.size(); vpid++) {
        auto const pvNode = pvNodes[vpid];
        if (!vpvkmer[vpid]) continue;
        auto const &kmers = *vpvkmer[vpid].get();
        auto const &TID = *vTID[vpid].get();
        for (unsigned int i = 0 ; i < kmers.size(); i++) {
            vector<uint32_t> ret;
            vector<uint8_t> retmap;
            bool respond = pvNode->smartQuery(&kmers[i], ret, retmap);
            if (pans->count(TID[i]) == 0) {
                vector<int> empty(high+1);
                pans->emplace(TID[i], empty);
            }
            if (respond) {
                auto &vec = pans->at(TID[i]);
                for (auto &p : ret) vec[p] ++;
                //for (auto &p : ret) pans->[TID[i]][p]++;
            }
            else {
                for (int v = 0; v< high; v++) { //ONLY EXP =1...
                    auto &vec = pans->at(TID[i]);
                    if (retmap[v>>3] & ( 1<< (v & 7)))
                        vec[v]++;
                }
            }
            /*
            printf("Update pans %d:", TID[i]);

            ConstantLengthKmerHelper<uint64_t, uint16_t> helper(D_KMERLENGTH,0);
                printf( "%12llx:", kmers[i]);
                char buf[30];
                memset(buf,0,sizeof(buf));
                uint64_t k = kmers[i];
                helper.convertstring(buf,&k);
                printf( "%s ", buf);

            for (int v = 0 ; v < high; v++)
                printf("%d ", pans->at(TID[i]).at(v));
            printf("\n");
            */
        }

    }
    printf("L2 thread %d finished\n", myid);
};



int main(int argc, char ** argv) {
    args::ArgumentParser parser("Query SeqOthello! \n");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<string> argSeqOthName(parser, "string", "the path name of seqOthello", {"map"});
    args::ValueFlag<string> argTranscriptName(parser, "string", "file containing transcripts", {"transcript"});
    args::ValueFlag<string> resultsName(parser, "string", "where to put the results", {"output"});
    args::Flag   argShowDedatils(parser, "", "Show the detailed query results for the transcripts", {"detail"});
    args::Flag   NoReverseCompliment(parser, "",  "do not use reverse complement", {"noreverse"});
    args::ValueFlag<int>  argNQueryThreads(parser, "int", "how many threads to use for query, default = 1", {"qthread"});


    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help)
    {
        std::cout << parser;
        return 0;
    }
    catch (args::ParseError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    catch (args::ValidationError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    if (!(argSeqOthName && argTranscriptName && resultsName)) {
        std::cerr << "must specify args" << std::endl;
        return 1;
    }

    bool flag = !args::get(NoReverseCompliment);
    
    if (argNQueryThreads) {
        nqueryThreads = args::get(argNQueryThreads);
    }

    std::shared_ptr<SeqOthello> seqoth;
    string filename = args::get(argSeqOthName);
    seqoth = make_shared<SeqOthello> (filename, nqueryThreads ,false);
    int kmerLength = seqoth->kmerLength;
    FILE *fin;
    fin = fopen64(args::get(argTranscriptName).c_str(),"rb");
    char buf[1048576];
    memset(buf,0,sizeof(buf));
    vector<string> vSeq;
    while ( fgets(buf,sizeof(buf),fin)!= NULL) {
        char * p;
        p = &buf[0];
        while (*p == 'A' || *p == 'T' || *p == 'G' || *p == 'C' || *p == 'N') p++;
        *p = '\0';
        if (strlen(buf)>=3)
            vSeq.push_back(string(buf));
    }
    fclose(fin);
    int nSeq = vSeq.size();
    string fnameout = args::get(resultsName);

    FILE *fout = fopen(fnameout.c_str(), "w");

    if (argShowDedatils) {
	    seqoth->loadAll(nqueryThreads);
        ConstantLengthKmerHelper<uint64_t, uint16_t> helper(kmerLength,0);
        vector<uint64_t>  requests;
        set<uint32_t> skipped;

        for (unsigned int id = 0; id < vSeq.size(); id++)  {
            auto &str = vSeq[id];
            int ul = str.size()-kmerLength+1;
            if (ul<=0) {
                ul = 0;
                skipped.insert(id);
            }
            char buf[64];
            memset(buf,0,sizeof(buf));
            if (ul>0)
                for (unsigned int i = 0 ; i < str.size() - kmerLength + 1; i++) {
                    memcpy(buf,str.data()+i,kmerLength);
                    uint64_t key;
                    helper.convert(buf,&key);
                    requests.push_back(key);
                    if (flag) {
                        key = helper.minSelfAndRevcomp(key);
                    }
                }
        }

        for (auto k : requests) {
            vector<uint32_t> vret;
            vector<uint8_t> vmap;
            char buf[30];
            memset(buf,0,sizeof(buf));
            helper.convertstring(buf,&k);
            fprintf(fout, "%s ", buf);
            bool res = seqoth->smartQuery(&k, vret, vmap);
            if (!res) {
                vret.clear();
                for (unsigned int v = 0; v< seqoth->sampleCount; v++) {
                    if (vmap[v>>3] & ( 1<< (v & 7)))
                        vret.push_back(v);
                }
            }
            set<uint16_t> vset(vret.begin(), vret.end());
            for (unsigned int i = 0; i < seqoth->sampleCount; i++) {
                if (vset.count(i)) fprintf(fout, "+");
                else fprintf(fout, ".");
            }
            fprintf(fout, "\n");
        }
        return 0;
    }

    seqoth->loadL1(kmerLength);
    vector<vector<string>> vtSeq(nqueryThreads);
    vector<vector<int>> vtTID(nqueryThreads);
    for (unsigned int id = 0; id< vSeq.size(); id++) {
        vtSeq[id % nqueryThreads].push_back(vSeq[id]);
        vtTID[id % nqueryThreads].push_back(id);
    }
    vSeq.clear();
    vector<vector<uint64_t>> vKmer(nqueryThreads);
    vector<vector<uint16_t>> vL1Result(nqueryThreads);
    vector<vector<uint32_t>> vkTID(nqueryThreads);
    vector<shared_ptr<thread>> L1threads;
    for (int i = 0 ; i < nqueryThreads; i++) {
        auto th1 = make_shared<thread>(getL1Result, seqoth, std::ref(vtSeq[i]), std::ref(vtTID[i]), std::ref(vL1Result[i]), std::ref(vKmer[i]), std::ref(vkTID[i]), flag);
        L1threads.push_back(th1);
    }
    for (auto &th: L1threads)
        th->join();

    vtSeq.clear();
    vtTID.clear();
    seqoth->releaseL1();
    seqoth->startloadL2(nqueryThreads);
    unsigned int vnodecnt = seqoth->vNodes.size();
    vector<shared_ptr<vector<uint64_t>>> vL2kmer(vnodecnt, nullptr);
    vector<shared_ptr<vector<uint32_t>>> vL2TID(vnodecnt, nullptr);
    uint32_t L2IDShift = seqoth->L2IDShift;
    map<int, vector<int> *> ans;
    for (int i = 0 ; i < nSeq; i++)
        ans.emplace(i, new vector<int> (seqoth->L2IDShift));
    for (int i = 0 ; i < nqueryThreads; i++) {
        for (unsigned int j = 0 ; j < vL1Result[i].size(); j++) {
            uint16_t othquery = vL1Result[i][j];
            if (othquery == 0) continue;
            if (othquery < L2IDShift) {
                ans.find(vkTID[i][j])->second->at(othquery-1) ++;
                continue;
            }
            if (othquery - L2IDShift >= vnodecnt)
                continue;
            if (vL2kmer[othquery - L2IDShift ] == nullptr) {
                vL2kmer[othquery - L2IDShift] = make_shared<vector<uint64_t>>();
                vL2TID[othquery - L2IDShift] = make_shared<vector<uint32_t>>();
            }
            vL2kmer[othquery - L2IDShift]->push_back(vKmer[i][j]);
            vL2TID[othquery - L2IDShift]->push_back(vkTID[i][j]);
        }
    }
    vL1Result.clear();
    vKmer.clear();
    vkTID.clear();
    seqoth->waitloadL2();
    vector<shared_ptr<map<int, vector<int>>>> response;
    response.reserve(vnodecnt);
    printf("Splitting into L2 groups\n");
    vector<shared_ptr<thread>> L2threads;
    vector<vector<shared_ptr<L2Node>>> vvpNode(nqueryThreads);
    vector<vector<shared_ptr<vector<uint64_t>>>> vpkmergrp(nqueryThreads);
    vector<vector<shared_ptr<vector<uint32_t>>>> vpTIDgrp(nqueryThreads);
    for (unsigned int i = 0 ; i < vnodecnt; i++) {
        int tid = i % nqueryThreads;
        vvpNode[tid].push_back( seqoth->vNodes[i]);
        vpkmergrp[tid].push_back(vL2kmer[i]);
        vpTIDgrp[tid].push_back(vL2TID[i]);
    }

    for (int i = 0; i < nqueryThreads; i++) {
        //map<int, vector<int>> empty;
        response.push_back(make_shared<map<int,vector<int>>>());
        auto th = make_shared<thread>
                  (getL2Result,seqoth->sampleCount, vvpNode[i], std::ref(vpkmergrp[i]), std::ref(vpTIDgrp[i]), response[i].get());
        L2threads.push_back(th);
    }
    for (auto &th : L2threads)
        th->join();
    vvpNode.clear();
    vpkmergrp.clear();
    vpTIDgrp.clear();
    printf("Gather L2 results\n");
    for (auto const &res : response)
        for (auto const &resKv : *res.get()) {
            if (ans.count(resKv.first) == 0)
                ans[resKv.first] = new vector<int>(resKv.second);
            else {
                auto ip = ans[resKv.first]->begin();
                auto iq = resKv.second.begin();
                while (iq != resKv.second.end()) {
                    *ip += *iq;
                    ip++;
                    iq++;
                }
            }
        }
    printf("Printing\n");
    for (auto &res : ans) {
        fprintf(fout,"transcript# %d\t", res.first);
        for (unsigned int i = 0 ; i < seqoth->sampleCount; i++)
            fprintf(fout, "%d\t", (*res.second)[i]);
        fprintf(fout, "\n");
        delete res.second;
    }
    /*
    for (int i = 0 ; i < response.size(); i++) {
        printf("Results from response %d \n", i);
        for (auto const &resKv : *response[i].get()) {
            printf("%d :", resKv.first);
            for (int h = 0 ; h < seqoth->high; h++)
                printf("%d ", resKv.second[h]);
            printf("\n");
        }
    }
    */
    for (unsigned int i = 0 ; i < response.size(); i++) {
        response[i]->clear();
    }
    fclose(fout);

    // split the kmers onto multiple queues.
    // Step1: each thread gets vectors: v<seq> returns two v<kmer> v<L1Result>
    //         --- after this we have  v<v<kmer>> v<v<L1Result>>  v<v<transciptID>>
    //         --- categorize them into v<v<kmer>> v<v<transcriptID>> by L1 result.
    // Step2: for each node: get v<kmer>,
}
