// This file is a part of SeqOthello. Please refer to LICENSE.TXT for the LICENSE
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <bitset>
#include <chrono>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <inttypes.h>
#include <args.hxx>
#include <io_helper.hpp>
#include <tinyxml2.h>

using namespace std;

int getKmerLengthfromxml(string fname,bool argGroup) {
    fname += ".xml";
    tinyxml2::XMLDocument doc;
    doc.LoadFile( fname.c_str() );
    int ret = 0;
    if (argGroup) {
        const tinyxml2::XMLElement * pSampleInfo = doc.FirstChildElement( "Root" )->FirstChildElement( "GroupInfo" );
        pSampleInfo->QueryIntAttribute("KmerLength", &ret);
    } else {
        const tinyxml2::XMLElement * pSampleInfo = doc.FirstChildElement( "Root" )->FirstChildElement( "SampleInfo" );
        pSampleInfo->QueryIntAttribute("KmerLength", &ret);
    }
    return ret;
}
int main(int argc, char ** argv) {
    args::ArgumentParser parser("Use a subset of the experiments to build the SeqOthello Group file.");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<string> argFname(parser, "string", "a file containing the filenames of the binary files. Each line of the [flist] must contain exactly one file name, e.g, xxxx.bin", {"flist"});
    args::ValueFlag<string> argFolder(parser, "string", "The directory to the output binary files.", {"folder"});
    args::ValueFlag<string> argOut(parser, "string", "The filename of the output Group file.", {"output"});
    args::ValueFlag<int> argMaxKmerCount(parser, "integer", "The maximum number of kmers used to build the Group file. Note: this function is for testing purpose only.", {"limit"});
    args::Flag argGroup(parser,"","Create a group using some group files (debugging only).", {"group"});
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
        // std::cerr << "Must specify args. Try --help." << std::endl;
        std::cerr << parser;
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
    if (fnames.size()>250) {
        std::cerr << "Too many files in the group." << std::endl;
        return 1;
    }

    //check kmer length consistent;
    set<int> kmerlengthset;
    for (auto s:fnames) {
        int kl = getKmerLengthfromxml(s, argGroup);
        kmerlengthset.insert(kl);
    }
    if (kmerlengthset.size() > 1) {
        std::cerr << "Kmer length not consistent." << std::endl;
        return 1;
    }
    if (fnames.size() == 0) {
        std::cerr << "fail to get kmer filelist." << std::endl;
        return 1;

    }
    int KmerLength = *kmerlengthset.begin();
    std::function<bool(uint64_t &, vector<uint32_t> &)> func;
    if (!argGroup) {
        auto const reader = new KmerGroupReader<	uint64_t, BinaryKmerReader<uint64_t>    >(fnames);
        func = std::bind(& KmerGroupReader< uint64_t, BinaryKmerReader<uint64_t>>::getNextValueList, reader, placeholders::_1, placeholders::_2);
    }
    else {
        auto const reader = new KmerGroupComposer<uint64_t>(fnames);
        func = std::bind( & KmerGroupComposer<uint64_t>::getNextValueList, reader, placeholders::_1, placeholders::_2);
    }
    uint64_t k;
    vector<uint32_t> ret;
    auto writer = new MultivalueFileReaderWriter<uint64_t, uint8_t> (args::get(argOut).c_str(), sizeof(uint64_t), sizeof(uint8_t), false);

    vector<uint64_t> vhistogram(16384,0);
    uint64_t cnt = 0;
    while (func(k, ret) && (limit -- >0)) {
        vhistogram[ret.size()] ++;
        sort(ret.begin(), ret.end());
        vector<uint8_t> res;
        res.reserve(ret.size());
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
    int filecnt = 0;
    auto pRoot = xml.NewElement("Root");
    auto pSamples = xml.NewElement("Samples");
    for (auto &fname : fnames) {
        string sampleXmlF = fname +".xml";
        printf("%s\n", sampleXmlF.c_str());
        tinyxml2::XMLDocument doc;
        doc.LoadFile( sampleXmlF.c_str() );
        if (!argGroup) {
            filecnt++;
            const tinyxml2::XMLElement * pSampleInfo = doc.FirstChildElement( "Root" )->FirstChildElement( "SampleInfo" );
            string str;
            const auto attr = pSampleInfo->FindAttribute("KmerFile");
            if (attr)
                printf("query: %s\n", attr->Value());
            tinyxml2::XMLNode * cpyNode = pSampleInfo->DeepClone(&xml);
            pSamples->InsertEndChild(cpyNode);
        }
        else {
            tinyxml2::XMLElement * pSamplesFrom = doc.FirstChildElement( "Root" )->FirstChildElement( "Samples" );
            for (tinyxml2::XMLElement * child = pSamplesFrom->FirstChildElement("SampleInfo"); child != NULL; child = child->NextSiblingElement()) {
                tinyxml2::XMLNode * cpyNode = child->DeepClone(&xml);
                pSamples->InsertEndChild(cpyNode);
                filecnt++;
            }

        }
    }
    auto pGroupInfo = xml.NewElement("GroupInfo");
    pGroupInfo->SetAttribute("TotalSamples", (uint32_t) filecnt);
    pGroupInfo->SetAttribute("KmerLength", (uint32_t) KmerLength);
    pGroupInfo->SetAttribute("GroupFile", args::get(argOut).c_str() );
    pGroupInfo->SetAttribute("Keycount", (int64_t) cnt);
    pRoot->InsertFirstChild(pGroupInfo);
    auto pHistogram = xml.NewElement("Histogram");
    for (int i = 1; i< 16384; i++)
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
    cout << "wrote "<<cnt<<" keys" << endl;
    return 0;
}
