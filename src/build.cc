#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <bitset>
#include <chrono>
#include <inttypes.h>
#include <string>
#include <map>
#include <unordered_map>
#include <args.hxx>
#include <memory>
#include <io_helper.hpp>
#include <oltnew.h>

using namespace std;


int main(int argc, char ** argv) {
    args::ArgumentParser parser("Build SeqOthello! \n");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<string> argInputname(parser, "string", "a file containing the filenames, these files should be created by preprocess", {"flist"});
    args::ValueFlag<string> argFolder(parser, "string", "where to find this file", {"folder"});
    args::ValueFlag<string> argOutputname(parser, "string", "filename (including path) for the output files", {"out"});
    args::ValueFlag<int> argThread(parser, "int", "number of parallel threads to build SeqOthello", {"thread"});
    args::ValueFlag<int> argLimit(parser, "int", "read this number of Kmers to estimate the distribution.", {"estimate-limit"});
    args::Flag argCountOnly(parser, "count-only", "only count the keys and the histogram, do not build the seqOthello.", {"count-only"});
//    args::ValueFlag<int> argEXP(parser, "int", "Expression bits, optional: None, 1, 2, 4", {"exp"});


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
    if (!(argInputname && argFolder && argOutputname)) {
        std::cerr << "must specify args" << std::endl;
        return 1;
    }
    int nThreads = 1;
    if (argThread)
        nThreads = args::get(argThread);
    vector<uint64_t> keyHisto, encodeHisto;

    string prefix = args::get(argFolder);
    string fname =  args::get(argInputname);
    FILE * ffnames = fopen(args::get(argInputname).c_str(), "r");
    if (ffnames == NULL) 
        throw std::invalid_argument("Error reading file"+argInputname);
    char buf[4096];
    vector<string> fnames;
    while (true) {
        if (fgets(buf, 4096, ffnames) == NULL) break;
        string fname(buf);
        if (*fname.rbegin() == '\n') fname = fname.substr(0,fname.size()-1);
        fnames.push_back(prefix+fname);
    }
    auto reader = make_shared<KmerGroupComposer<uint64_t>>(fnames);
    reader->verbose = true;
    vector<uint32_t> ret;
    vector<uint8_t> encodebuf;

    uint32_t samplecount = reader->gethigh();
    printf("samplecount = %d\n", samplecount);
    keyHisto.resize(samplecount+5);
    encodeHisto.resize(samplecount+5);
    tinyxml2::XMLDocument * xml = new tinyxml2::XMLDocument();

    if (argCountOnly) {
        uint64_t k= 0;
        int64_t cnt = 0;
        while (reader->getNextValueList(k, ret)) {
            int keycnt = ret.size();
            if (keycnt > samplecount) {
                printf("%d \n", keycnt);
                for (auto &x: ret)
                    printf("%d ", x);

                printf("\n");
            }
            keyHisto[keycnt]++;
            cnt ++;
        }
        auto pRoot = xml->NewElement("Root");

        auto pcountInfo = xml->NewElement("KeyDistributionInfo");
        pcountInfo->SetAttribute("TotalKeycount", cnt);
        for (int i = 0; i < reader->gethigh(); i++)
            if (keyHisto[i]) {
                auto pHisNode = xml->NewElement("entry");
                pHisNode->SetAttribute("freq", i);
                pHisNode->SetAttribute("value", (uint32_t) keyHisto[i]);
                pcountInfo->InsertEndChild(pHisNode);
            }
        pRoot->InsertEndChild(pcountInfo);

        xml->InsertFirstChild(pRoot);
        string output = args::get(argOutputname);
        xml->SaveFile(output.c_str());
        return 0;
    }
    int limit = 10485760;
    if (argLimit)
        limit = args::get(argLimit);
    printf("Estimate the distribution with the first %d Kmers. \n", limit);
    auto distr = SeqOthello::estimateParameters(reader.get(), limit);
    for (int i = 0 ; i < distr.size(); i++) {
        printf("%d->%d\n", i, distr[i]);
    }
    reader->reset();
//    auto reader = make_shared<GrpReader<uint64_t>> (args::get(argInputname), args::get(argFolder));
    auto seqoth = make_shared<SeqOthello> ();

    seqoth->constructFromReader(reader.get(), args::get(argOutputname), nThreads, distr);
    return 0;
}
