#include <cstdio>
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>
#include <args.hxx>
#include <io_helper.hpp>
#include <tinyxml2.h>

using namespace std;
int main(int argc, char * argv[]) {
    args::ArgumentParser parser("Convert a Jellyfish output file to a sorted binary file. ", "");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<string> argInputname(parser, "string", "filename for the input kmer file", {"in"});
    args::ValueFlag<string> argOutputname(parser, "string", "filename for the output binary kmer file", {"out"});
    args::ValueFlag<int> argKmerlength(parser, "integer", "k, length of kmer", {"k"});
    args::ValueFlag<int> nCutoff(parser, "integer", "cutoff, minimal expression value for kmer to be included into the file. ", {"cutoff"});

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
    if (!(argInputname && argOutputname && argKmerlength)) {
        std::cerr << "must specify args" << std::endl;
        return 1;
    }

    int kmerlength = args::get(argKmerlength);
    
    ConstantLengthKmerHelper<uint64_t, uint32_t> iohelper(kmerlength,0);

    vector<uint64_t> VKmer;
    FileReader<uint64_t, uint32_t> *freader;
    string finName = args::get(argInputname);
    string foutName = args::get(argOutputname);
    uint32_t cutoff = 0;
    if (nCutoff)
            cutoff = args::get(nCutoff);
    printf("Read files from %s\n", finName.c_str());
    freader = new KmerFileReader< uint64_t,uint32_t > (finName.c_str(), &iohelper,false);
    uint32_t minInputExpression = 0x7FFFFFFF;
    uint64_t k; uint32_t v;
    while (freader->getNext(&k, &v)) {
        if (v < minInputExpression) 
            minInputExpression = v;
        if (v >= cutoff) 
            VKmer.push_back(k);
    }
    if (VKmer.size() >0 ) {
        printf("Sorting %lu keys\n", VKmer.size());
        sort(VKmer.begin(),VKmer.end());
    }
    else {
        printf("Empty kmer files\n");
    }
    unsigned long long cnt = 0;
    if (VKmer.size()) {
    cnt ++;
    for (unsigned int i = 1; i < VKmer.size(); i++ ) {
       if (VKmer[i] != VKmer[cnt-1]) {
           VKmer[cnt] = VKmer[i];
           cnt++;
       }
    }
    }
    printf("Writing %lld keys to %s\n", cnt, foutName.c_str());
    FILE *fout = fopen(foutName.c_str(),"wb");
    fwrite(&VKmer[0], cnt, sizeof(VKmer[0]), fout);
    fclose(fout);

    tinyxml2::XMLDocument xml;
    auto pRoot = xml.NewElement("Root");
    auto pElement = xml.NewElement("SampleInfo");
    pElement->SetAttribute("KmerFile", finName.c_str());
    pElement->SetAttribute("KmerLength", kmerlength);
    pElement->SetAttribute("BinaryFile", foutName.c_str());
    pElement->SetAttribute("KmerCount", (unsigned int) VKmer.size());
    if (cnt) {
    pElement->SetAttribute("Cutoff", cutoff);
    pElement->SetAttribute("MinExpressionInKmerFile", minInputExpression);
    pElement->SetAttribute("UniqueKmerCount",(unsigned int) cnt);
    }
    pRoot->InsertEndChild(pElement);
    xml.InsertFirstChild(pRoot);
    auto xmlName = foutName + ".xml";
    xml.SaveFile(xmlName.c_str());
    return 0;
}
