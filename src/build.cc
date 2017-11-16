// This file is a part of SeqOthello. Please refer to LICENSE.TXT for the LICENSE
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
    args::ValueFlag<string> argInputname(parser, "string", "a file containing the filenames of Grp files, these Grp files should be created by the Preprocess tool. Each line should contain one file name. ", {"flist"});
    args::ValueFlag<string> argFolder(parser, "string", "where to find these Grp files. i.e. , a path that contains the Grp files. ", {"folder"});
    args::ValueFlag<string> argOutputname(parser, "string", "a folder to put the generated SeqOthello map.", {"out-folder"});
    //args::ValueFlag<int> argThread(parser, "int", "number of parallel threads to build SeqOthello", {"thread"});
    args::ValueFlag<int> argLimit(parser, "int", "read this number of Kmers to estimate the distribution.", {"estimate-limit"});
    args::Flag argCountOnly(parser, "count-only", "only count the keys and the histogram, do not build the seqOthello.", {"count-only"});
    //args::ValueFlag<int> argEXP(parser, "int", "Expression bits, optional: None, 1, 2, 4", {"exp"});


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
    //if (argThread)
    //    nThreads = args::get(argThread);
    vector<uint64_t> keyHisto, encodeHisto;

    string prefix = "";
    if (argFolder) prefix = args::get(argFolder);
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
        vector< vector <uint32_t> > detailedHisto(samplecount, vector<uint32_t>(samplecount+1, 0));
        while (reader->getNextValueList(k, ret)) {
            uint32_t keycnt = ret.size();
            if (keycnt > samplecount) {
                printf("%d \n", keycnt);
                for (auto &x: ret)
                    printf("%d ", x);
                printf("\n");
            }
            keyHisto[keycnt]++;
            for (auto &x: ret)
                detailedHisto[x][keycnt]++;
            cnt ++;
        }
        auto pRoot = xml->NewElement("Root");

        auto pcountInfo = xml->NewElement("KeyDistributionInfo");
        pcountInfo->SetAttribute("TotalKeycount", cnt);
        for (unsigned int i = 0; i < reader->gethigh(); i++)
            if (keyHisto[i]) {
                auto pHisNode = xml->NewElement("entry");
                pHisNode->SetAttribute("freq", i);
                pHisNode->SetAttribute("value", (uint32_t) keyHisto[i]);
                pcountInfo->InsertEndChild(pHisNode);
            }
        pRoot->InsertEndChild(pcountInfo);

        xml->InsertFirstChild(pRoot);
        string output = "keydistribution.xml";
        xml->SaveFile(output.c_str());
        auto vres = reader->getSampleInfo();
        FILE *fout = fopen("histo.txt","w");
        for (int i = 0 ; i < samplecount; i++) {
            fprintf(fout, "%s,", vres[i].c_str());
            for (int j = 1; j<=samplecount; j++)
                fprintf(fout, "%d,", detailedHisto[i][j]);
            fprintf(fout,"\n");
        }
        fclose (fout);
        return 0;
    }
    int limit = 10485760;
    if (argLimit)
        limit = args::get(argLimit);
    printf("Estimate the distribution with the first %d Kmers. \n", limit);
    uint64_t keycount = 0;
    auto distr = SeqOthello::estimateParameters(reader.get(), limit, keycount);
    /*
    for (int i = 0 ; i < distr.size(); i++) {
        printf("%d->%d\n", i, distr[i]);
    }*/
    printf("We estimate there are %lu keys\n", keycount);
    reader->reset();
//    auto reader = make_shared<GrpReader<uint64_t>> (args::get(argInputname), args::get(argFolder));
    auto seqoth = make_shared<SeqOthello> ();

    seqoth->constructFromReader(reader.get(), args::get(argOutputname), nThreads, distr, keycount);
    return 0;
}
