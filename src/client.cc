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
#include <atomic>
#include "PracticalSocket.h"  // For Socket, ServerSocket, and SocketException

using namespace std;


int main(int argc, char ** argv) {
    args::ArgumentParser parser("The client to query SeqOthello from a server \n");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<string> argTranscriptName(parser, "string", "file containing transcripts", {"transcript"});
    args::ValueFlag<string> resultsName(parser, "string", "where to put the results", {"output"});
    args::ValueFlag<int>  argServerPort(parser, "int", "connect to SeqOthello Server at port ", {"port"});
    args::ValueFlag<string>  argServerAdd(parser, "string", "start a SeqOthello Server at address (default: localhost)", {"server"});
    args::Flag   argInteractive(parser, "",  "start interactive CLI", {"interactive"});

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
    FILE *fin, *fout;
    if (!argInteractive) {
        if (!(argTranscriptName && resultsName && argServerPort)) {
            std::cerr << "must specify args" << std::endl;
            return 1;
        }
        fin = fopen64(args::get(argTranscriptName).c_str(),"rb");
        string fnameout = args::get(resultsName);
        fout = fopen(fnameout.c_str(), "w");

        if (fin == NULL) {
            std:: cerr << "Error reading file " << args::get(argTranscriptName) << std::endl;
            return 1;
        }
    }
    else {
        if ((!argServerPort) || argTranscriptName || resultsName) {
            std::cerr << "wrong arg" << std::endl;
            return 1;
        }
        fin = stdin;
        fout = stdout;
    }

    char buf[1048576];
    memset(buf,0,sizeof(buf));

    try {
        string servadd = "localhost";
        if (argServerAdd) servadd = args::get(argServerAdd);
        TCPSocket sock(servadd.c_str(), args::get(argServerPort));
        printf("Connecting to %s : %d\n", servadd.c_str(), args::get(argServerPort));
        while ( fgets(buf,sizeof(buf),fin)!= NULL) {
            char * p;
            p = &buf[0];
            while (*p == 'A' || *p == 'T' || *p == 'G' || *p == 'C' || *p == 'N') p++;
            *p = '\0';
            if (strlen(buf)<=3) break;
            int strl;
            sock.sendmsg(buf, strl = strlen(buf));
            printf("Sent a query with %d bases.\n", strl );
            int bytesReceived = 0;
            int totReceived = 0;
            bool flag = false;
            string buf;
            int tot = 0 ;
            while (sock.recvmsg(buf)) {
                if (buf.size()==0) break;
                fprintf(fout, "%s\n", buf.c_str());
                tot += buf.size();
            }
            if (!argInteractive)
                printf("received reponse of %d Bytes.\n", tot);
        }
    }
    catch(SocketException &e) {
        fclose(fin);
        fclose(fout);
        cerr << e.what() << endl;
        exit(1);
    }
    fclose(fin);
    fclose(fout);




}
