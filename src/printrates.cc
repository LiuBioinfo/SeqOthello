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


int main(int argc, char ** argv) {
    args::ArgumentParser parser("Query SeqOthello! \n");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<string> argSeqOthName(parser, "string", "the path contains SeqOthello mapping file.", {"map-folder"});

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

    SeqOthello * seqoth;
    string filename = args::get(argSeqOthName);
    seqoth = new SeqOthello (filename, 16 ,false);
    seqoth->loadAll(16);
    seqoth->printrates();
}
