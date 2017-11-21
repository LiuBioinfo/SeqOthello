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

int queryL2InThreadPool(int i, const shared_ptr<SeqOthello> seqoth,
                        const vector<uint64_t> &kmers,
                        const vector<uint32_t> &TIDs,
                        const vector<uint32_t> &PosInTranscript,
                        vector<vector<shared_ptr<string>>> &detailans,
                        map<int, vector<int> *> &ans,
                        bool argShowDedatils,
                        int high,
                        std::mutex &mu
                       ) {
    printf("L2 Load %d.\n", i); 

    seqoth->loadL2Node(i);
    if (!seqoth->vNodes[i]) return 0;

    map<int, vector<int> > mans;
    map<pair<int,int>, string> mstr;
    L2Node*  pvNode = (seqoth->vNodes[i]).get(); 
//            int high = seqoth->sampleCount;
    printf("L2 got %d kmers.\n", kmers.size()); 
    for (int j = 0 ; j < kmers.size(); j++) {
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
    printf("finished %d kmers.\n", kmers.size()); 
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
    seqoth->releaseL2Node(i);
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
    args::ArgumentParser parser("Query SeqOthello! \n");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<string> argSeqOthName(parser, "string", "the path contains SeqOthello mapping file.", {"map-folder"});
    args::ValueFlag<string> argTranscriptName(parser, "string", "file containing transcripts", {"transcript"});
    args::ValueFlag<string> resultsName(parser, "string", "where to put the results", {"output"});
    args::Flag   argShowDedatils(parser, "", "Show the detailed query results for the transcripts", {"detail"});
    args::Flag   NoReverseCompliment(parser, "",  "do not use reverse complement", {"noreverse"});
    args::ValueFlag<int>  argNQueryThreads(parser, "int", "how many threads to use for query, default = 1", {"qthread"});

    args::ValueFlag<int>  argStartServer(parser, "int", "start a SeqOthello Server at port ", {"start-server-port"});

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

    if (argStartServer) {
        if (argTranscriptName || argTranscriptName || !argSeqOthName || argNQueryThreads) {
            std::cerr <<" Invalid args. to start a server, please specify SeqOthello mapping file." << std:: endl;
            return 1;
        }
    }
    else {
        if (!(argSeqOthName && argTranscriptName && resultsName)) {
            std::cerr << "must specify args" << std::endl;
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

    unsigned int vnodecnt = seqoth->vNodes.size();
    vector<shared_ptr<vector<uint64_t>>> vL2kmer(vnodecnt, nullptr);
    vector<shared_ptr<vector<uint32_t>>> vL2TID(vnodecnt, nullptr);
    vector<shared_ptr<vector<uint32_t>>> vL2KmerPosInTranscript(vnodecnt, nullptr);
    uint32_t L2IDShift = seqoth->L2IDShift;
    map<int, vector<int> *> ans;
    for (int i = 0 ; i < nSeq; i++)
        ans.emplace(i, new vector<int> (seqoth->L2IDShift));

//    vector<shared_ptr<unordered_map<int, vector<int>>>> response;
    vector<vector<uint64_t>> seqInKmers;
    vector<vector<bool>> usedreverse;
    vector<vector<shared_ptr<string>>> detailans;
    ConstantLengthKmerHelper<uint64_t, uint16_t> helper(kmerLength,0);
    int totallength = 0;
    for (auto &str : vSeq)  {
        int ul = str.size()-kmerLength+1;
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

    for (int i = 0; i < L1Resp.size(); i++) {
        for (int j = 0; j < L1Resp[i].size(); j++) {
            uint16_t othquery = L1Resp[i].at(j);
            if (othquery ==0) continue;
            if (othquery < L2IDShift) {
                ans.find(i)->second->at(othquery-1) ++;
                if (argShowDedatils) {
                    string str(seqoth->sampleCount,'.');
                    str[othquery-1] = '+';
                    detailans[i][j] = make_shared<string>(str);
                }
                continue;
            }
            if (othquery - L2IDShift >= vnodecnt) continue;
            if (vL2kmer[othquery - L2IDShift ] == nullptr) {
                vL2kmer[othquery - L2IDShift] = make_shared<vector<uint64_t>>();
                vL2TID[othquery - L2IDShift] = make_shared<vector<uint32_t>>();
                if (argShowDedatils)
                    vL2KmerPosInTranscript[othquery - L2IDShift] = make_shared<vector<uint32_t>>();
            }
            vL2kmer[othquery - L2IDShift]->push_back(seqInKmers[i].at(j));
            vL2TID[othquery - L2IDShift]->push_back(i);
            if (argShowDedatils)
                vL2KmerPosInTranscript[othquery - L2IDShift]->push_back(j);
        }
    }
    L1Resp.clear();
    ThreadPool pool(nqueryThreads, 1024);
    std::vector<std::future<int>> results;

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
        char buf[30];
        memset(buf,0,sizeof(buf));
        for (int i = 0 ; i < seqInKmers.size(); i++) {
            vector<shared_ptr<string>> &vans = detailans[i];
            for (int j = 0 ; j < seqInKmers[i].size(); j++) {
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
    }
    fclose(fout);

}
