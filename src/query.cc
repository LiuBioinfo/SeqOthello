// This file is a part of SeqOthello. Please refer to LICENSE.TXT for the LICENSE
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
#include "socket.h"

#include <threadpool.h>

using namespace std;

int nqueryThreads = 1;

struct ThreadParameter {
    TCPSocket * sock;
    SeqOthello *oth;
    string iobuf;
};
int queryL2ShowSampleonly(int i, const shared_ptr<SeqOthello> seqoth,
                          const vector<uint64_t> &kmers,
                          const vector<uint32_t> &TIDs,
                          const vector<uint32_t> &PosInTranscript,
                          vector<shared_ptr<vector<int>>> & ansSampleDetails,
                          int showSampleIndex,
                          unsigned int high,
                          std::mutex &mu) {
    printf("L2 Load %d.\n", i);
    seqoth->loadL2Node(i);
    if (!seqoth->vNodes[i]) return 0;
    L2Node*  pvNode = (seqoth->vNodes[i]).get();
    printf("L2 got %lu kmers -- try to get sample index %d.\n", kmers.size(), showSampleIndex);
    unordered_map<int, vector<int>> mans;
    for (unsigned int j = 0 ; j < kmers.size(); j++) {
        auto kmer = kmers[j];
        auto TID = TIDs[j];
        vector<uint32_t> ret;
        vector<uint8_t> retmap;
        bool respond = pvNode->smartQuery(&kmer, ret, retmap);
        if (respond) {
            if (find(ret.begin(), ret.end(), showSampleIndex)!=ret.end())
                mans[TID].push_back(PosInTranscript[j]);
        }
        else {
            if (retmap[showSampleIndex >> 3] &( 1<< ( showSampleIndex &7)))
                mans[TID].push_back(PosInTranscript[j]);
        }
    }
    seqoth->releaseL2Node(i);
    printf("finished %lu kmers.\n", kmers.size());
    {
        std::unique_lock<std::mutex> lock(mu);
        for (auto &x: mans) {
            if (ansSampleDetails[x.first] == nullptr)
                ansSampleDetails[x.first] = make_shared<vector<int>>();
            auto & refv = *ansSampleDetails[x.first];
            for (auto &y: x.second)
                refv.push_back(y);
        }
    }
    printf("L2 Release %d.\n", i);
}

int queryL2InThreadPool(int i, const shared_ptr<SeqOthello> seqoth,
                        const vector<uint64_t> &kmers,
                        const vector<uint32_t> &TIDs,
                        const vector<uint32_t> &PosInTranscript,
                        vector<vector<shared_ptr<string>>> &detailans,
                        map<int, vector<int> *> &ans,
                        bool argShowDedatils,
                        unsigned int high,
                        std::mutex &mu
                       ) {
    printf("L2 Load %d.\n", i);

    seqoth->loadL2Node(i);
    if (!seqoth->vNodes[i]) return 0;

    map<int, vector<int> > mans;
    map<pair<int,int>, string> mstr;
    L2Node*  pvNode = (seqoth->vNodes[i]).get();
//            int high = seqoth->sampleCount;
    printf("L2 got %lu kmers.\n", kmers.size());
    for (unsigned int j = 0 ; j < kmers.size(); j++) {
        auto kmer = kmers[j];
        auto TID = TIDs[j];
        vector<uint32_t> ret;
        vector<uint8_t> retmap;
        bool respond = pvNode->smartQuery(&kmer, ret, retmap);
        if (mans.count(TID) == 0) {
            vector<int> empty(high+1);
            mans.emplace(TID, empty);
        }
        if (argShowDedatils) {
            string str(high,'.');
            if (respond) {
                for (auto &p : ret)
                    if (p<high)
                        str[p]='+';
            }
            else {
                auto vec = mans.at(TID);
                for (uint32_t v = 0; v< high; v++) { //ONLY EXP =1...
                    if (retmap[v>>3] & ( 1<< (v & 7)))
                        str[v] = '+';
                }
            }
            mstr[make_pair(TID,PosInTranscript[j])] = str;
        }
        else {
            if (respond) {
                auto &vec = mans.at(TID);
                for (auto &p : ret)
                    if (p<=high)
                        vec[p] ++;
            }
            else {
                auto &vec = mans.at(TID);
                for (uint32_t v = 0; v< high; v++) { //ONLY EXP =1...
                    if (retmap[v>>3] & ( 1<< (v & 7)))
                        vec[v]++;
                }
            }
        }
    }
    seqoth->releaseL2Node(i);
    printf("finished %lu kmers.\n", kmers.size());
    {
        std::unique_lock<std::mutex> lock(mu);
        if (argShowDedatils) {
            for (auto &x: mstr) {
                detailans[x.first.first][x.first.second] = make_shared<string>(x.second);
            }
        }
        else {
            for (auto &x: mans) {
                if (ans.count(x.first) == 0)
                    ans[x.first] = new vector<int>(x.second);
                else {
                    auto ip = ans[x.first]->begin();
                    auto iq = x.second.begin();
                    while (iq != x.second.end()) {
                        *ip  += *iq;
                        ip++;
                        iq++;
                    }
                }
            }
        }
    }
    printf("L2 Release %d.\n", i);
    return 0;
}



void process(const string &type, ThreadParameter *par) {
    char ans[65536];
    vector<int> queryans;
    static int CONTAINMENT = 1;
    static int COVERAGE = 2;
    int query_type;
    if (type == TYPE_CONTAINMENT)
        query_type = CONTAINMENT;
    else if (type == TYPE_COVERAGE)
        query_type = COVERAGE;
    else {
        string ans = "Must specify query type";
        par->sock->sendmsg(ans);
        par->sock->sendmsg("");
        return;
    }
    if (query_type == CONTAINMENT)
        queryans.resize(par->oth->sampleCount);
    uint32_t kmerLength = par->oth->kmerLength;
    ConstantLengthKmerHelper<uint64_t, uint16_t> helper(kmerLength,0);
    auto &str = par->iobuf;
    if (str.size()<kmerLength) {
        string ans = "transcript "+ par->iobuf + "is too short.";
        par->sock->sendmsg(ans);
        par->sock->sendmsg("");
        return;
    }
    char buf[32];
    memset(buf,0,sizeof(buf));
    vector<uint64_t>  requests;
    vector<bool> usedreverse;
    for (unsigned int i = 0 ; i < str.size() - kmerLength + 1; i++) {
        memcpy(buf,str.data()+i,kmerLength);
        uint64_t key = 0 ,key0 = 0 ;
        helper.convert(buf,&key);
        key = helper.minSelfAndRevcomp(key0 = key);
        usedreverse.push_back(key == key0);
        requests.push_back(key);
    }

    auto  itUsedrevse = usedreverse.begin();
    for (auto k : requests) {
        vector<uint32_t> vret;
        vector<uint8_t> vmap;
        memset(ans,'x',sizeof(ans));
        auto toconvert = k;
        if (*itUsedrevse)
            toconvert = helper.reverseComplement(k);
        itUsedrevse++;
        helper.convertstring(ans,&toconvert);
        char *p = & ans[kmerLength];
        *p = ' ';
        p++;
        bool res = par->oth->smartQuery(&k, vret, vmap);
        if (!res) {
            vret.clear();
            for (unsigned int v = 0; v< par->oth->sampleCount; v++) {
                if (vmap[v>>3] & ( 1<< (v & 7)))
                    vret.push_back(v);
            }
        }
        if (query_type == COVERAGE) {
            set<uint16_t> vset(vret.begin(), vret.end());
            for (unsigned int i = 0; i < par->oth->sampleCount; i++) {
                if (vset.count(i)) *p = '+';
                else *p = '.';
                p++;
            }
            *p ='\0';
            par->sock->sendmsg(ans, strlen(ans));
        }
        else {
            for (auto x: vret) if (x<queryans.size())
                    queryans[x] ++;
        }
    }
    if (query_type == CONTAINMENT) {
        stringstream ss;
        for (auto x:queryans)
            ss <<x <<" ";
        string str =ss.str();
        par->sock->sendmsg(str);
    }
    par->sock->sendmsg("");
}

void HandleTCPClient(ThreadParameter *par) {
    cout << "Handling client ";
    try {
        cout << par->sock->getForeignAddr() << ":";
    } catch (std::runtime_error e) {
        cerr << "Unable to get foreign address" << endl;
    }
    try {
        cout << par->sock->getForeignPort();
    } catch (std::runtime_error e) {
        cerr << "Unable to get foreign port" << endl;
    }
    cout << " with thread " << pthread_self() << endl;

    // Send received string and receive again until the end of transmission
    string str;
    try {
        while (par->sock->recvmsg(str)) {
            // end of transmission
            if (str == TYPE_CONTAINMENT || str == TYPE_COVERAGE) {
                par->sock->recvmsg(par->iobuf);
                cout << str << std::endl;
                if (par->iobuf.size()>0) {
                    process(str, par);
                }
            }
        }
    } catch( std::runtime_error e) {
        cerr << "Error while responding..." << e.what()<< endl;
    }
    // Destructor closes socket

}

void *ServerThreadMain(void *par) {
    // Guarantees that thread resources are deallocated upon return
    pthread_detach(pthread_self());

    // Extract socket file descriptor from argument
    HandleTCPClient((ThreadParameter *) par);

    delete (TCPSocket *)( ((ThreadParameter* )par)->sock);
    return NULL;
}

int main(int argc, char ** argv) {
    args::ArgumentParser parser("Transcript query on SeqOthello.\n");
    args::HelpFlag help(parser, "help", "Display the help menu.", {'h', "help"});
    args::ValueFlag<string> argSeqOthName(parser, "string", "The directory to SeqOthello map.", {"map-folder"});
    args::ValueFlag<string> argTranscriptName(parser, "string", "The filename of the transcript fasta file.", {"transcript"});
    args::ValueFlag<string> resultsName(parser, "string", "The filename of the output.", {"output"});
    args::Flag   argShowDedatils(parser, "", "Return detailed k-mer presence/absence information for the transcript, limited to one trascript per query.", {"detail"});
    args::Flag   NoReverseCompliment(parser, "",  "Do not use reverse complement.", {"noreverse"});
    args::ValueFlag<int>  argNQueryThreads(parser, "int", "how many threads to use for query, default = 1.", {"qthread"});

    args::ValueFlag<int>  argStartServer(parser, "int", "start a SeqOthello Server at port.", {"start-server-port"});
    args::ValueFlag<int>  argSampleIndex(parser, "int", "printout kmers that matches a sample with index.", {"print-kmers-index"});

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

    bool flag = !args::get(NoReverseCompliment);
    int showSampleIndex = -1;
    if (argSampleIndex) {
        if (argShowDedatils || argStartServer) {
            std::cerr <<" Invalid args." << std::endl;
            return 1;
        }
        showSampleIndex = args::get(argSampleIndex);
    }
    if (argStartServer) {
        if (argTranscriptName || argTranscriptName || !argSeqOthName || argNQueryThreads) {
            std::cerr <<" Invalid args. to start a server, please specify SeqOthello mapping file." << std:: endl;
            return 1;
        }
    }
    else {
        if (!(argSeqOthName && argTranscriptName && resultsName)) {
            // std::cerr << "must specify args" << std::endl;
            std::cerr << parser;
            return 1;
        }
    }

    if (argNQueryThreads) {
        nqueryThreads = args::get(argNQueryThreads);
    }


    shared_ptr<SeqOthello>  seqoth;
    string filename = args::get(argSeqOthName);
    seqoth = make_shared<SeqOthello> (filename, nqueryThreads ,false);
    if (argStartServer) {
        printf("Load SeqOthello. \n");
        seqoth->loadAll(nqueryThreads);
        unsigned short echoServPort =  args::get(argStartServer);
        printf("SeqOthello Loaded. Now start Server at port %d\n", echoServPort);

        try {
            TCPServerSocket servSock(echoServPort);   // Socket descriptor for server
            for (;;) {      // Run forever
                TCPSocket *clntSock = servSock.accept();
                ThreadParameter p;
                p.sock = clntSock;
                p.oth = seqoth.get();
                pthread_t threadID;              // Thread ID from pthread_create()
                if (pthread_create(&threadID, NULL, ServerThreadMain,
                                   (void *) (&p)) != 0) {
                    cerr << "Unable to create thread" << endl;
                    exit(1);
                }
            }
        } catch (std::runtime_error e) {
            cerr << e.what() << endl;
            exit(1);
        }
        // NOT REACHED
        return 0;
    }

    int kmerLength = seqoth->kmerLength;
    FILE *fin = NULL;
#ifdef __APPLE__
    fin = fopen(args::get(argTranscriptName).c_str(),"rb");
#else
    fin = fopen64(args::get(argTranscriptName).c_str(),"rb");
#endif
    if (fin == NULL)
        throw std::invalid_argument("Error while opening file "+args::get(argTranscriptName));
    char buf[1048576];
    memset(buf,0,sizeof(buf));
    vector<string> vSeq;
    while ( fgets(buf,sizeof(buf),fin)!= NULL) {
        char * p;
        p = &buf[0];
        if (strlen(buf) > 1000000) {
            throw std::invalid_argument("Transcript too long, we only support 1000000.");
        }
        while (*p == 'A' || *p == 'T' || *p == 'G' || *p == 'C' || *p == 'N') p++;
        *p = '\0';
        if (strlen(buf)>=3)
            vSeq.push_back(string(buf));
    }
    fclose(fin);
    unsigned int nSeq = vSeq.size();
    string fnameout = args::get(resultsName);
    FILE *fout = fopen(fnameout.c_str(), "w");
    if (fin == NULL)
        throw std::invalid_argument("Error while opening file "+(fnameout));

    unsigned int vnodecnt = seqoth->vNodes.size();
    vector<shared_ptr<vector<uint64_t>>> vL2kmer(vnodecnt, nullptr);
    vector<shared_ptr<vector<uint32_t>>> vL2TID(vnodecnt, nullptr);
    vector<shared_ptr<vector<uint32_t>>> vL2KmerPosInTranscript(vnodecnt, nullptr);
    uint32_t L2IDShift = seqoth->L2IDShift;
    map<int, vector<int> *> ans;
    for (unsigned int i = 0 ; i < nSeq; i++)
        ans.emplace(i, new vector<int> (seqoth->L2IDShift));

//    vector<shared_ptr<unordered_map<int, vector<int>>>> response;
    vector<vector<uint64_t>> seqInKmers;
    vector<vector<bool>> usedreverse;
    vector<vector<shared_ptr<string>>> detailans;
    ConstantLengthKmerHelper<uint64_t, uint16_t> helper(kmerLength,0);
    int totallength = 0;
    set<int> skipped;
    int id = -1;
    for (auto &str : vSeq)  {
        int ul = str.size()-kmerLength+1;
        id++;
        if (ul<1)  {
            printf("Skipping transcript # %d, %s\n", id, str.c_str());
            skipped.insert(id);
            continue;
        }
        char buf[64];
        memset(buf,0,sizeof(buf));
        vector<uint64_t> kmers;
        vector<bool> reverse;
        if (ul>0)
            for (unsigned int i = 0 ; i < str.size() - kmerLength + 1; i++) {
                memcpy(buf,str.data()+i,kmerLength);
                uint64_t key = 0;
                helper.convert(buf,&key);
                uint64_t key0 = key;
                if (flag) {
                    key =  helper.minSelfAndRevcomp(key);
                    reverse.push_back(key == key0);
                }
                kmers.push_back(key);
            }
        if (flag)
            usedreverse.push_back(reverse);
        seqInKmers.push_back(vector<uint64_t>(kmers));
        totallength += kmers.size();
        if (argShowDedatils) {
            vector<shared_ptr<string>> strs(ul);
            detailans.emplace_back(strs);
        }
        if (argShowDedatils && totallength > 128*1024) {
            throw std::invalid_argument("Too many bases in the transcripts for coverage query. Please do less than 128K kmers per time.");
        }
    }

    auto L1Resp = seqoth->QueryL1ByPartition(seqInKmers, nqueryThreads);
    vector<shared_ptr<vector<int>>> ansSampleDetails(vSeq.size(), nullptr);
    for (unsigned int i = 0; i < L1Resp.size(); i++) {
        for (unsigned int j = 0; j < L1Resp[i].size(); j++) {
            uint16_t othquery = L1Resp[i].at(j);
            if (othquery ==0) continue;
            if (othquery < L2IDShift) {
                ans.find(i)->second->at(othquery-1) ++;
                if (argShowDedatils) {
                    string str(seqoth->sampleCount,'.');
                    str[othquery-1] = '+';
                    detailans[i][j] = make_shared<string>(str);
                }
                if (showSampleIndex >=0)
                    if (othquery-1 == showSampleIndex) {
                        if (ansSampleDetails[i] == nullptr)
                            ansSampleDetails[i] = make_shared<vector<int>>();
                        ansSampleDetails[i]->push_back(j);
                    }
                continue;
            }
            if (othquery - L2IDShift >= vnodecnt) continue;
            if (vL2kmer[othquery - L2IDShift ] == nullptr) {
                vL2kmer[othquery - L2IDShift] = make_shared<vector<uint64_t>>();
                vL2TID[othquery - L2IDShift] = make_shared<vector<uint32_t>>();
                if (argShowDedatils || showSampleIndex >=0)
                    vL2KmerPosInTranscript[othquery - L2IDShift] = make_shared<vector<uint32_t>>();
            }
            vL2kmer[othquery - L2IDShift]->push_back(seqInKmers[i].at(j));
            vL2TID[othquery - L2IDShift]->push_back(i);
            if (argShowDedatils || showSampleIndex >=0)
                vL2KmerPosInTranscript[othquery - L2IDShift]->push_back(j);
        }
    }
    L1Resp.clear();
    ThreadPool pool(nqueryThreads, 1024);
    std::vector<std::future<int>> results;
    if (argSampleIndex) {
        for (unsigned int i = 0 ; i < vnodecnt; i++)
            if (vL2kmer[i] != nullptr) {
                auto lambda = std::bind(
                                  queryL2ShowSampleonly,
                                  i,
                                  seqoth,
                                  std::ref(*vL2kmer[i]),
                                  std::ref(*vL2TID[i]),
                                  std::ref(*vL2KmerPosInTranscript[i]),
                                  std::ref(ansSampleDetails),
                                  ((int) showSampleIndex),
                                  seqoth->sampleCount,
                                  std::ref(pool.write_mutex)
                              );
                std::future<int> x = pool.enqueue(i, lambda);
                results.emplace_back(std::move(x));
            }
        for (auto && result: results)
            result.get();
        printf("Printing\n");
        ConstantLengthKmerHelper<uint64_t, uint16_t> helper(kmerLength,0);
        char buf[32];
        memset(buf,0,sizeof(buf));
        for (int i = 0 ; i < ansSampleDetails.size(); i++) {
            fprintf(fout,"Kmers Hit In Transcript %d\n", i);
            if (ansSampleDetails[i] == nullptr) continue;
            sort(ansSampleDetails[i]->begin(), ansSampleDetails[i]->end());
            for (int j = 0 ; j < ansSampleDetails[i]->size(); j++) {
                int id;
                uint64_t key = seqInKmers[i].at((id = ansSampleDetails[i]->at(j)));
                if (flag) if  (usedreverse[i][id]) {
                        key = helper.reverseComplement(key);
                    }
                helper.convertstring(buf,&key);
                fprintf(fout, "%s\n", buf);
            }
        }
        fclose(fout);
        return 0;
    }
    for (unsigned int i = 0; i < vnodecnt; i++)
        if (vL2kmer[i] != nullptr) {
//            queryL2InThreadPool(i, *pvNode, vL2kmer[i], vL2TID[i], detailans, ans, argShowDedatils,seqoth->high);
            auto lambda = std::bind(
                              queryL2InThreadPool,i,
                              seqoth,
                              std::ref(*vL2kmer[i]),
                              std::ref(*vL2TID[i]),
                              std::ref(*vL2KmerPosInTranscript[i]),
                              std::ref(detailans),
                              std::ref(ans),
                              ((bool) argShowDedatils),
                              seqoth->sampleCount,
                              std::ref(pool.write_mutex)
                          );
            std::future<int> x = pool.enqueue(i, lambda);
            results.emplace_back(std::move(x));

        }
    for (auto && result: results)
        result.get();

    printf("Printing\n");
    if (argShowDedatils) {
        ConstantLengthKmerHelper<uint64_t, uint16_t> helper(kmerLength,0);
        char buf[32];
        memset(buf,0,sizeof(buf));
        for (unsigned int i = 0 ; i < seqInKmers.size(); i++) {
            vector<shared_ptr<string>> &vans = detailans[i];
            for (unsigned int j = 0 ; j < seqInKmers[i].size(); j++) {
                uint64_t key = seqInKmers[i].at(j);
                if (flag)
                    if (usedreverse[i][j])
                        key = helper.reverseComplement(key);
                helper.convertstring(buf,&key);
                if (vans[j])  {
                    string str = *(vans[j].get());
                    fprintf(fout, "%s %s\n", buf, str.c_str());
                }//detailans[i].at(j).get()->c_str());
                else {
                    fprintf(fout, "%s %s\n", buf, string(seqoth->sampleCount,'.').c_str());
                }
            }
        }
    } else {
        int skippedcount = 0;
        for (auto &res : ans) {
            while (skipped.count(res.first + skippedcount)) {
                fprintf(fout,"transcript# %d\t", res.first+skippedcount);
                for (unsigned int i = 0 ; i < seqoth->sampleCount; i++)
                    fprintf(fout, "0\t");
                fprintf(fout,"\n");
                skippedcount ++;
            }
            fprintf(fout,"transcript# %d\t", res.first+skippedcount);
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
    }
    fclose(fout);

}
