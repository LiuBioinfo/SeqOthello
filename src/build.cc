#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <bitset>
#include <filegrouper.hpp>
#include <oltnew.h>
#include <chrono>
#include <inttypes.h>
#include <string>
#include <map>
#include <unordered_map>
#include <args.hxx>
#include <othellotypes.hpp>

using namespace std;


int main(int argc, char ** argv) {
    args::ArgumentParser parser("Build SeqOthello! \n");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<string> argInputname(parser, "string", "a file containing the filenames, these files should be created by preprocess", {"flist"});
    args::ValueFlag<string> argFolder(parser, "string", "where to find this file", {"folder"});
    args::ValueFlag<string> argOutputname(parser, "string", "filename (including path) for the output files", {"out"});
    args::ValueFlag<int> argThread(parser, "int", "number of parallel threads to build SeqOthello", {"thread"});
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

    int EXP;

    auto reader = make_shared<GrpReader<uint64_t>> (args::get(argInputname), args::get(argFolder));
    auto seqoth = make_shared<SeqOthello<uint64_t>> (args::get(argfcnt), 1, D_SPLITBIT, D_KMERLENGTH);

    seqoth->constructFromReader(reader.get(), args::get(argOutputname), nThreads);
    return 0;
}
