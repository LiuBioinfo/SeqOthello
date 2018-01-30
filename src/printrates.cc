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
    args::ArgumentParser parser("Query SeqOthello。 \n");
    args::HelpFlag help(parser, "help", "Display the help menu.", {'h', "help"});
    args::ValueFlag<string> argSeqOthName(parser, "string", "The directory to SeqOthello map.", {"map-folder"});
    args::ValueFlag<int> argL2(parser, "int", "L2 Node ID", {"L2"});
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
    if (!argL2) {
    seqoth->loadL1(seqoth->kmerLength);
    seqoth->printrates();
    }
    if (argL2) {
            int L2id = args::get(argL2);
            seqoth->loadL2Node(L2id);
            if (!seqoth->vNodes[L2id]) return 0;
            auto othProvalues = seqoth->vNodes[L2id]->getRates();
            auto retmap = seqoth->vNodes[L2id]->computeProb(othProvalues);
            int entrycnt = seqoth->vNodes[L2id]->getEntrycnt();
            double zeroRate =0;
            for (auto &x:othProvalues) {
                  if (x.first ==0 || x.first>entrycnt)
                          zeroRate += x.second;
            }
            printf("L2 Node %d, entrycnt %d\n", L2id, entrycnt);
            for (auto &x: retmap)
                    printf("%d %.8lf\n", x.first,x.second);
            printf("Zero %.8lf\n", zeroRate);
    }

}
