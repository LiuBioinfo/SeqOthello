#pragma once
#include <jellyfish_helper.hpp>

#include <config.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <limits>

#define HAVE_NUMERIC_LIMITS128

#include <jellyfish/err.hpp>
#include <jellyfish/misc.hpp>
//#include <jellyfish/fstream_default.hpp>
#include <jellyfish/jellyfish.hpp>


template <typename keyType, typename valueType> 
class JellyfishFileReader : public FileReader<keyType, valueType> {
    jellyfish::file_header header;
    std::ifstream ifs;
    ConstantLengthKmerHelper<keyType, valueType> *io_helper;
    binary_reader *reader;
public:    
    int kmerlength = 0; 
    JellyfishFileReader(const char * fname) : ifs(fname) {
        header.read(ifs);
        jellyfish::mer_dna::k(header.key_len() / 2);
        io_helper = new ConstantLengthKmerHelper<keyType, valueType>(header.key_len()/2, 0);
        kmerlength = header.key_len() / 2;
        reader = new binary_reader(ifs, &header);
    }
    void finish() {
    }
    void reset() {
       ifs.clear();
       ifs.seekg(0);
       header.read(ifs); 
    }
    bool getNext(keyType *T, valueType *V) {
        if (!reader->next()) return false;
//        stringstream ss; ss<< reader->key();
//        string s;
//        ss >> s;
        *T = *(reader->key().data());
        *V = reader->val();
//        keyType T2; valueType V2;
//        char buf[60];
//        strcpy(buf, s.c_str());
//        io_helper->convert(buf, &T2, &V2);
//        cout << "Reader: " << reader->key();
//        cout << "Reader key:" << *T << " Reconstruct" << T2 << endl;
    }
    bool getFileIsSorted() {
        return false;
    }
};
