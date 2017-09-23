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
#include <io_helper.hpp>
#include <tinyxml2.h>

using namespace std;


int main(int argc, char ** argv) {
    args::ArgumentParser parser("Preprocess binary files to grouped files. \n"
                                "Each line of the file must contain exactly one file name, e.g, xxxx.bin\n"
                                "The file should be in 64bit kmer format, the xml file must present in the same folder. xxxx.bin.xml" , "");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<string> argFname(parser, "string", "a file containing the filenames", {"flist"});
    args::ValueFlag<string> argFolder(parser, "string", "where to find this file", {"folder"});
    args::ValueFlag<string> argOut(parser, "string", "output file", {"output"});
    args::ValueFlag<int> argMaxKmerCount(parser, "integer", "stop after getting this number of kmers. Note: for test small data set only.", {"limit"});
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
    if (!(argFname && argFolder && argOut)) {
        std::cerr << "Must specify args. Try --help." << std::endl;
        return 1;
    }
    int limit = 0x7FFFFFFF;
    if (argMaxKmerCount) {
        limit = args::get(argMaxKmerCount);
    }
    string prefix = args::get(argFolder);
    FILE * ffnames = fopen(args::get(argFname).c_str(), "r");
    char buf[4096];
    vector<string> fnames;
    while (true) {
        if (fgets(buf, 4096, ffnames) == NULL) break;
        string fname(buf);
        if (*fname.rbegin() == '\n') fname = fname.substr(0,fname.size()-1);
        fnames.push_back(prefix+fname);
    }
    int from = 0;
    if (fnames.size()>250) {
        std::cerr << "Too many files in the group." << std::endl;
        return 1;
    }
    auto const reader = new KmerGroupReader<
        uint64_t, BinaryKmerReader<uint64_t>
    >(fnames);
    uint64_t k;
    vector<uint32_t> ret;
    auto writer = new MultivalueFileReaderWriter<uint64_t, uint8_t> (args::get(argOut).c_str(), sizeof(uint64_t), sizeof(uint8_t), false);

    vector<uint64_t> vhistogram(255,0);
    uint64_t cnt;

    while (reader->getNextValueList(k, ret) && (limit -- >0)) {
        vhistogram[ret.size()] ++;
        sort(ret.begin(), ret.end());
        vector<uint8_t> res; res.reserve(ret.size());
        for (auto &x: ret) res.push_back(x);
        writer->write(&k,res);
        /*for (auto &r: ret)
            r.first+= from;
        writer->write(k, ret);*/
        /*printf("%llx\t", k);
        for (auto &x: ret) 
                printf("%d ", x);
        printf("\n");*/
        cnt++;
    }
    writer->finish();
    tinyxml2::XMLDocument xml;
    auto pRoot = xml.NewElement("Root");
    auto pSamples = xml.NewElement("Samples");
    for (auto &fname : fnames) {
        string sampleXmlF = fname +".xml";
        printf("%s\n", sampleXmlF.c_str());
        tinyxml2::XMLDocument doc;
        doc.LoadFile( sampleXmlF.c_str() );
        const tinyxml2::XMLElement * pSampleInfo = doc.FirstChildElement( "Root" )->FirstChildElement( "SampleInfo" );
        string str;
        const auto attr = pSampleInfo->FindAttribute("KmerFile");
        if (attr) 
                printf("query: %s\n", attr->Value());
        tinyxml2::XMLNode * cpyNode = pSampleInfo->DeepClone(&xml);
        pSamples->InsertEndChild(cpyNode);
    }
    auto pGroupInfo = xml.NewElement("GroupInfo");
    pGroupInfo->SetAttribute("TotalSamples", (uint32_t) fnames.size());
    pGroupInfo->SetAttribute("GroupFile", args::get(argOut).c_str() );
    pRoot->InsertFirstChild(pGroupInfo);
    auto pHistogram = xml.NewElement("Histogram");
    for (int i = 1; i< 255; i++)
        if (vhistogram[i]) {
            auto pHisNode = xml.NewElement("entry");
            pHisNode->SetAttribute("freq", i);
            pHisNode->SetAttribute("value", (uint32_t) vhistogram[i]);
            pHistogram->InsertEndChild(pHisNode);
        }
    pRoot->InsertEndChild(pHistogram);
    pRoot->InsertEndChild(pSamples);
    xml.InsertFirstChild(pRoot);
    auto xmlName = args::get(argOut) + ".xml";
    xml.SaveFile(xmlName.c_str());

    delete writer;
    cout << "wrote "<<cnt<<"keys";
    return 0;
}
