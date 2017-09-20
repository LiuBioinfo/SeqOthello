#include <iostream>
#include <cstdio>
#include <vector>
#include <args.hxx>
#include <io_helper.h>
#include <set>
#include <othellotypes.h>
int main(int argc, char ** argv) {

    args::ArgumentParser parser("Generate a list of kmer data (k=20) for the purpose of testing.", "");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<int> nFiles(parser, "integer", "number of files", {'f'});
    args::ValueFlag<int> nKmers(parser, "integer", "number of kmers", {'k'});

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

    int files = (nFiles)?(args::get(nFiles)):(20);
    int kmers = (nKmers)?(args::get(nKmers)):(100);
    auto helper = new ConstantLengthKmerHelper<uint64_t, uint16_t>(20,0);


    uint64_t start_state = 0x185abd8c71u;  /* Any nonzero start state will work. */
    uint64_t lfsr = start_state;
    uint64_t bit;                    /* Must be 16bit to allow bit<<15 later in the code */
    unsigned period = 0;




    char buf[32];
    memset(buf,0,sizeof(buf));
    uint32_t curr;
    uint64_t vv = 0;
    std::set<uint64_t> sss;
    std::vector<uint64_t> vkmer;
    std::string str;
    char fname[30];
    memset(fname,0,sizeof(fname));
    for (uint64_t i = 1; i< kmers+files; i++) {
        // x^40 + x ^ 38 + x^ 21 + x^9
        bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 19) ^ (lfsr >> 21) ) & 1;
        lfsr =  (lfsr >> 1) | (bit << 39);
        bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 19) ^ (lfsr >> 21) ) & 1;
        lfsr =  (lfsr >> 1) | (bit << 39);
        vv = lfsr;
        sss.insert(vv);
        helper->convertstring(buf,&vv);
        if (i == kmers+files) {
            std::string tot(buf);
            str = tot + str;
        }
        else {
            std::string std=" ";
            std[0] = buf[19];
            str = std + str;
            vkmer.push_back(vv);
        }
    }
    for (int i = 1; i<= files; i++) {
        char fname[30];
        memset(fname,0,sizeof(fname));
        sprintf(fname,"F%d.Kmer", i);
        FILE *fout = fopen(fname ,"w");
        char buf[30];
        memset(buf,0,sizeof(buf));
        for (int j = i-1; j<i+kmers-1; j++) {
            if ((j+i)%(i*i/50+3)<=1) continue;
            helper->convertstring(buf,&vkmer[j]);
            fprintf(fout, "%s %d\n", buf, (j-i+2)*i);
        }
        fclose(fout);
    }
    printf("%lud %d\n", sss.size(), kmers+files);
    std::cout << str << endl;
    return 0;

}
