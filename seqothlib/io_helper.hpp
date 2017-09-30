#pragma once
/*!
 * \file io_helper.h
 * Contains IO utilities.
 */
#include <string>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <errno.h>
#include <cstring>
#include <queue>
#include <algorithm>
#include <cstring>
#include <set>
#include <bitset>
#include <ctime>
#include <tinyxml2.h>
#include "util.h"

using namespace std;
/*!
 * \brief interface for converting a key from its raw format to a keyTypeype. Split key into groups.
 */
template <typename keyType, typename valueType>
class IOHelper {
public:
    /*!
       \brief convert a input-style line to key/value pair.
       \param [in] char *s, a line from the input.
       \param [out] keyType* T, converted key.
       \param [out] valueType* V, converted value.
       \retval boolean return true if convert is success.
      */
    virtual bool convert(char *s, keyType *T, valueType *V)  = 0;
    //! \brief skip the value.
    virtual bool convert(char *s, keyType *T)  = 0;

    virtual keyType reverseComplement(keyType T) = 0;
    virtual void convertstring( char *s, keyType *T) = 0;

    /*!
     \brief split a keyTypeype value into two parts: groupID/keyInGroup by the highest *splitbit* bits.
    */
    virtual void splitgrp(const keyType &key, uint32_t &grp, keyType &keyInGroup) = 0;
    /*!
     \brief combine groupID/keyInGroup to the origional key
     */
    virtual void combgrp(keyType &key, uint32_t &grp, keyType &keyInGroup) = 0;
};

template<typename keyType, typename valueType>
class FileReader {
public:
    IOHelper<keyType, valueType> *helper;
    virtual bool getFileIsSorted() = 0;
    virtual bool getNext(keyType *T, valueType *V) = 0;
    virtual void finish() =0;
    virtual void reset() = 0;
    virtual ~FileReader() {
    }
};

/*!
 * \brief IOHelper for Constant-Length Kmers.
 * \note Each Kmer is a string of length KMERLENGTH, \n
 * We consider string as a base-4 number. [A=0,C=1,G=2,T=3]. \n
 * Kmers are grouped according to the highest *splitbit* bits.
 */
template <typename keyType, typename valueType>
class ConstantLengthKmerHelper : public IOHelper<keyType,valueType> {
public:
    uint8_t kmerlength; //!< Assume all kmers are of the same length.
    uint8_t splitbit;   //!< group the keys according to the highest bits.
    ConstantLengthKmerHelper(uint8_t _kmerlength, uint8_t _splitbit): kmerlength(_kmerlength),splitbit(_splitbit) {};
    ~ConstantLengthKmerHelper() {};
    unsigned char ReverseBitsInByte(unsigned char v)
    {
        return (v * 0x0202020202ULL & 0x010884422010ULL) % 1023;
    }
    void convertstring(char *s, keyType *T) {
        std::string str;
        keyType key = *T;
        for (int i = 0 ; i < kmerlength; i++) {
            str.push_back("ACGT"[key & 3]);
            key >>=2;
        }
        std::reverse(str.begin(),str.end());
        strcpy(s, str.c_str());

    }
    inline keyType reverseComplement(keyType k) {
        if (k & 0x8000000000000000ULL) return k;
        keyType ans = 0ULL;
        for (uint32_t i = 0 ; i < sizeof(keyType); i++) {
            ans <<=8;
            ans |= ReverseBitsInByte( (unsigned char) k);
            k>>=8;
        }
        keyType qq = ((~ (ans ^ (ans>>1))) & 0x5555555555555555ULL);
        ans = (ans^(qq*3));
        ans >>= (sizeof(keyType)*8 - kmerlength*2);
        return ans;
    }
    keyType minSelfAndRevcomp(keyType k) {
        keyType comp = reverseComplement(k);
        return (comp < k)? comp : k;
    }
    inline bool convert(char *s, keyType *k, valueType *v) {
        switch (*s) {
        case 'A':
        case 'T':
        case 'G':
        case  'C':
            keyType ret = 0;
            while (*s == 'A' || *s == 'C' || *s =='T' || *s =='G' || *s == 'N') { //A = 0, C == 1, G == 2, T == 3, N:kmer = 0x8000000000000000ULL;
                ret <<=2;
                if (*s == 'N') return 0x8000000000000000ULL;
                switch (*s) {
                case 'T':
                    ret++;
                case 'G':
                    ret++;
                case 'C':
                    ret++;
                }
                s++;
            }
            *k = ret;
            valueType tv;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
            sscanf(s,"%u",&tv);
#pragma GCC diagnostic pop
            *v = tv;
            return true;

        }
        return false;

    }
    inline bool convert( char *s, keyType *k) {
        valueType v;
        return convert(s,k,&v);
    }
    void splitgrp(const keyType &key, uint32_t &grp, keyType &keyInGroup) {
        int mvcnt = 2 * kmerlength - splitbit;
        keyType high = (key >> mvcnt);
        grp = high;
        keyType lowmask = 1;
        lowmask <<= mvcnt;
        keyInGroup = (key & (lowmask-1));
    }

    void combgrp(keyType &key, uint32_t &grp, keyType &keyInGroup) {
        key = grp;
        key <<= (2*kmerlength - splitbit);
        key |= (keyInGroup);
    }

};



template <typename keyType, typename valueType>
class KmerFileReader : public FileReader<keyType,valueType> {
    FILE *f;
    bool fIsSorted;
public:
    KmerFileReader(const char *fname, IOHelper<keyType,valueType> *_helper, bool b)  {
        fIsSorted = b;
        FileReader<keyType,valueType>::helper = _helper;
        char buf[1024];
        strcpy(buf,fname);
        if (buf[strlen(buf)-1]=='\n')
            buf[strlen(buf)-1] = '\0';
        f=fopen(buf,"r");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
        printf("OpenFile %s %x\n",fname,f);
#pragma GCC diagnostic pop
    }
    void finish() {
        fclose(f);
    }
    void reset() {
        rewind(f);
    }
    bool getFileIsSorted() {
        return fIsSorted;
    }
    ~KmerFileReader() {
        finish();
    }
    bool getNext(keyType *T, valueType *V) {
        char buf[1024];
        if (fgets(buf,sizeof(buf),f)==NULL) return false;
        return FileReader<keyType,valueType>::helper->convert(buf,T,V);
    }
};

template <typename keyType>
class KmerReader {
public:
    virtual void finish() =0;
    virtual bool getNext(keyType *k) =0;
    virtual void reset() = 0;
};

template <typename keyType>
struct KIDpair {
    keyType k;
    uint32_t id;
    bool finished;
    bool friend operator <( const KIDpair &a, const KIDpair &b) {
        if (a.finished != b.finished) return (((int) a.finished) > ((int) b.finished));
        if (a.k != b.k)
            return a.k>b.k;
        return a.id > b.id;
    }
};

template <typename keyType, class KmerReader>
class KmerGroupReader {
    vector<KmerReader *> readers;
    priority_queue<KIDpair<keyType>> PQ;
protected:
    bool hasNext = true;

public:
    KmerGroupReader() {}
    KmerGroupReader(vector<string> & fnames) {
        for (auto const fname : fnames) {
            readers.push_back(new KmerReader(fname.c_str()));
            keyType k;
            readers[readers.size()-1]->getNext(&k);
            KIDpair<keyType> kid = {k, (uint32_t) (readers.size()-1), false};
            PQ.push(kid);
        }
    }
    ~KmerGroupReader() {
        for (int i = 0 ; i < readers.size(); i++)
            delete readers[i];
    }

    bool verbose = false;
    uint64_t keycount = 0;
    bool getNextValueList(keyType &k, vector<uint32_t> &ret) {
        k = PQ.top().k;
        if (PQ.top().finished) {
            return false;
        }
        ret.clear();
        while (PQ.top().k == k && !PQ.top().finished) {
            int tid;
            tid = PQ.top().id;
            keyType nextk;
            ret.push_back(tid);
            bool finish = !readers[tid]->getNext(&nextk);
            PQ.pop();
            KIDpair<keyType> kid = {nextk, (uint32_t) tid, finish};
            PQ.push(kid);
            updatekeycount();
        }
        return true;
    }
protected:
    void updatekeycount() {
        keycount ++;
        if (keycount > 100000 && verbose)
            if ((keycount & (keycount-1))==0) {
                printcurrtime();
                printf("Got %lld keys\n", keycount);
            }
    }
};

template <typename keyType, typename valueType>
class KmerFilteredFileReader : public FileReader<keyType, valueType> {
    KmerFileReader<keyType, valueType> *freader;
    int lowerbound;
public:
    KmerFilteredFileReader(KmerFileReader<keyType, valueType> * _freader, int _lowerbound)  {
        freader = _freader;
        lowerbound = _lowerbound;
    }
    void finish() {
        freader->finish();
    }
    void reset() {
        freader->reset();
    }
    bool getFileIsSorted() {
        return freader->getFileIsSorted();
    }
    ~KmerFilteredFileReader() {
        freader->finish();
    }
    bool getNext(keyType *T, valueType *V) {
        while (true) {
            if (!freader->getNext(T,V)) return false;
            if (*V >= lowerbound) return true;
        }
    }
};


template <typename keyType, typename valueType>
struct KVpair {
    keyType k;
    valueType v;
    bool friend operator <( const KVpair &a, const KVpair &b) {
        return a.k > b.k;
    }
} __attribute__((packed));



template <typename keyType, typename valueType>
class compressFileReader : public FileReader <keyType, valueType> {
    FILE *f;
    bool fIsSorted;
    static const int buflen = 1024*8;
    int curr = 0;
    int  max = 0;
    unsigned char buf[1024*64];
    uint32_t kl, vl;
public:
    compressFileReader( const char * fname, IOHelper <keyType, valueType> * _helper, uint32_t klength, uint32_t valuelength, bool _fIsSorted = true) {
        kl = klength;
        vl = valuelength;
        FileReader<keyType,valueType> :: helper = _helper;
        fIsSorted = _fIsSorted;
        char buf[1024];
        strcpy(buf,fname);
        if (buf[strlen(buf)-1]=='\n')
            buf[strlen(buf)-1] = '\0';
        f=fopen(buf,"rb");
        printf("OpenFile to binary read Kmers %s %x\n",fname,f);
    }
    void finish() {
        fclose(f);
    }
    void reset() {
        printf("Do not support reset()\n");
    }
    bool getFileIsSorted() {
        return fIsSorted;
    }
    ~ compressFileReader() {
        finish();
    }
    bool getNext(keyType *k, valueType *v) {
        if (curr == max) {
            max = fread(buf,kl+vl,buflen,f);
            if (max == 0) return false;
            curr = 0;
        }
        *k =0;
        *v =0;
        memcpy( (void *) k, buf + curr*(kl+vl), kl);
        memcpy( (void *) v, buf + curr*(kl+vl)+kl, vl);
        curr++;
        return true;
    }

};


template <typename keyType, typename valueType>
class MultivalueFileReaderWriter : public FileReader <keyType, valueType> {
    FILE *f;
    static const int buflen = 32768*8;
    int curr = 0;
    int max = 0;
    unsigned char buf[buflen * 2];
    uint32_t kl,vl;
    bool isRead;
    bool isclosed = false;
    unsigned long long keycount = 0;
public:
    static const valueType EMPTYVALUE = ~0;
    bool valid(valueType value) {
        if (vl == 1) return value!=0xFF;
        if (vl == 2) return value!=0xFFFF;
        if (vl == 4) return value!=0xFFFFFFFFUL;
        return false;
    }
    KmerReader<keyType> *writefilter;
    keyType nextPossibleKey;
    bool hasNext;
    unsigned long long passcount = 0;
    MultivalueFileReaderWriter( const char * fname, uint32_t klength, uint32_t valuelength, bool _isRead, KmerReader<keyType> * _writefilter = NULL) {
        kl = klength;
        vl = valuelength;
        char buf[1024];
        strncpy(buf,fname,1024);
        if (buf[strlen(buf)-1]=='\n')
            buf[strlen(buf)-1] = '\0';
        isRead = _isRead;
        if (isRead)
            f=fopen(buf,"rb");
        else f = fopen(buf,"wb");
        memset(buf,0,sizeof(buf));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
        printf("OpenFile to binary read/write Multivalue Kmers file %s %x\n",fname,f);
#pragma GCC diagnostic pop
        curr = 0;
        max = 0;
        if ((writefilter = _writefilter)!=NULL) {
            writefilter->reset();
            hasNext = writefilter->getNext(&nextPossibleKey);
        }
    }
    void finish() {
        if (!isclosed) {
            if (!isRead) {
                fwrite(buf,sizeof(buf[0]), curr, f);
            }
            fclose(f);
        }
        isclosed = true;
    }
    void reset() {
        rewind(f);
        memset(buf,0,sizeof(buf));
        curr = 0;
        max = 0;

    }
    virtual bool getFileIsSorted() {
        return false;
    }
    ~MultivalueFileReaderWriter() {
        finish();
    }
    void getmore() {
        memmove(buf, buf+curr, max - curr);
        max -= curr;
        curr = 0;
        if (max < buflen) {
            max += fread(buf+max,1,buflen, f);
        }
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
    bool get(void * mem, uint32_t l) {
        if (curr + l >= max) getmore();
        if (curr + l > max) return false;
        memcpy(mem,buf+curr,l);
        curr+=l;
        return true;
    }
#pragma GCC diagnostic pop
    virtual bool getNext(keyType *k, valueType *v) {
        if (!get(k,kl)) return false;
        while (true) {
            get(v,vl);
            if (!valid(*v)) return true;
            v++;
        }
        /*
        if (curr + kl+vl >= max) {
            getmore();
            if (max < kl) return false;
        }
        *k = 0;
        memcpy( (void *) k, buf + curr, kl);
        curr+=kl;
        memcpy( (void *) v, buf + curr, vl);
        curr += vl;
        while (valid(*v)) {
            v++;
            if (curr+ vl + vl >= max) getmore();
            memcpy( (void *) v, buf + curr, vl);
            curr += vl;
        }
        return true;
        */
    }
    void add(void * mem, uint32_t l) {
        memcpy(buf+curr, mem, l);
        curr += l;
        if (curr >= buflen) {
            fwrite( buf, 1, buflen, f);
            if (curr > buflen) {
                memcpy(buf,buf+buflen,curr-buflen);
            }
            curr -= buflen;
        }
    }
    bool testkey(keyType *k) {
        if (writefilter!= NULL) {
            while (nextPossibleKey < *k) {
                if (hasNext)
                    hasNext = writefilter->getNext(&nextPossibleKey);
                if (!hasNext) return false;
            }
            if (nextPossibleKey > *k) return false;
        }
        passcount ++;
        if (keycount > 1024) if ((keycount & (keycount - 1)) ==0) {
                printcurrtime();
                printf("Got %lld, passed %lld keys\n", keycount, passcount);
            }
        return true;
    }
    virtual bool write(keyType *k ,valueType *v) {
        keycount++;
        if (!testkey(k)) return false;
        add((void *) k, kl);
        while (valid(*v)) {
            add((void *) v, vl);
            v++;
        }
        return true;
    }
    bool write(keyType *k, std::vector<valueType> &vv) {
        keycount++;
        if (!testkey(k)) return false;
        add ( (void *) k, kl);
        void *p;
        uint8_t a8;
        uint16_t a16;
        uint32_t a32;
        if (vl == 1) p = &a8;
        if (vl == 2) p = &a16;
        if (vl == 4) p = &a32;
        for (auto v: vv) {
            a8 = a16 = a32 = v;
            add(p,vl);
        }
        a8 =0xFF;
        a16 = 0xFFFF;
        a32 = 0xFFFFFFFF;//~0ULL;
        add (p,vl);
    }
    unsigned long long getKeycount() {
        return keycount;
    }
    unsigned long long getpos() {
        return ftell(f);

    }
};



template <typename keyType>
class KmerGroupComposer {
    vector<MultivalueFileReaderWriter<uint64_t,uint8_t> *> readers;
    priority_queue<KIDpair<keyType>> PQ;
protected:
    bool hasNext = true;
    vector<vector<uint8_t>> tmpval;
    vector<uint32_t> shift;
    vector<uint64_t> totkeycount;
    vector<uint64_t> readkeys;
    int kmerlength;
    int getKmerLengthfromxml(string fname) {
        fname += ".xml";
        tinyxml2::XMLDocument doc;
        doc.LoadFile( fname.c_str() );
        const tinyxml2::XMLElement * pSampleInfo = doc.FirstChildElement( "Root" )->FirstChildElement( "GroupInfo" );
        int ret = 0;
        pSampleInfo->QueryIntAttribute("KmerLength", &ret);
        return ret;
    }
public:
    KmerGroupComposer() {}
    int32_t getKmerLength() {return kmerlength;}
    KmerGroupComposer(vector<string> & fnames) {

        //check kmer length consistent;
        set<int> kmerlengthset;
        for (auto s:fnames) {
            int kl = getKmerLengthfromxml(s);
            kmerlengthset.insert(kl);
        }
        if (kmerlengthset.size() > 1) {
            throw invalid_argument("KmerLength not consistent");
            return;
        }
        kmerlength = *kmerlengthset.begin();
        tmpval.resize(fnames.size());
        shift.push_back(0);
        for (int i = 0 ; i < fnames.size(); i++) {
            auto const fname = fnames[i];
            auto const fnamexml = fname + ".xml";
            tinyxml2::XMLDocument xml;
            printf("load xml info from %s\n", fnamexml.c_str());
            xml.LoadFile( fnamexml.c_str());
            int tmpint;
            xml.FirstChildElement("Root")->FirstChildElement("GroupInfo")->QueryIntAttribute("TotalSamples", &tmpint);
            int64_t tmpi64;
            xml.FirstChildElement("Root")->FirstChildElement("GroupInfo")->QueryInt64Attribute("Kmercount", &tmpi64);
            totkeycount.push_back(tmpi64);
            tmpval[i].resize(tmpint+1);
            shift.push_back(tmpint + *shift.rbegin());
            readers.push_back(new MultivalueFileReaderWriter<uint64_t, uint8_t>(fname.c_str(), sizeof(uint64_t), sizeof(uint8_t), true));
            keyType k;
            readers[readers.size()-1]->getNext(&k, &tmpval[i][0]);
            KIDpair<keyType> kid = {k, (uint32_t) (readers.size()-1), false};
            PQ.push(kid);
            readkeys.push_back(1);
        }
    }
    ~KmerGroupComposer() {
        for (int i = 0 ; i < readers.size(); i++)
            delete readers[i];
    }

    bool verbose = false;
    uint64_t keycount = 0;
    bool getNextValueList(keyType &k, vector<uint32_t> &ret) {
        k = PQ.top().k;
        if (PQ.top().finished) {
            return false;
        }
        ret.clear();
        while (PQ.top().k == k && !PQ.top().finished) {
            int tid;
            tid = PQ.top().id;
            keyType nextk;

            for (int i = 0; readers[tid]->valid(tmpval[tid][i]); i++) {
                ret.push_back(shift[tid] + tmpval[tid][i]);
            }
            bool finish = !readers[tid]->getNext(&nextk, &tmpval[tid][0]);
            readkeys[tid]++;
            PQ.pop();
            KIDpair<keyType> kid = {nextk, (uint32_t) tid, finish};
            PQ.push(kid);
            updatekeycount();
        }
        return true;
    }
    uint32_t gethigh() {
        return *shift.rbegin();
    }
    void getGroupStatus(vector<uint64_t> &curr, vector<uint64_t> &tot) {
        curr = readkeys;
        tot = totkeycount;
    }
    void reset() {
        fill(readkeys.begin(), readkeys.end(), 1);
        for (auto &x: readers)
            x->reset();
        keycount = 0;
        uint64_t k;
        PQ = priority_queue<KIDpair<keyType>>();

        for (int i = 0; i < readers.size(); i++) {
            readers[i]->getNext(&k, &tmpval[i][0]);
            KIDpair<keyType> kid = {k, i, false};
            PQ.push(kid);
            readkeys[i] = 1;
        }
    }
protected:
    void updatekeycount() {
        keycount ++;
        if ((0==(keycount & 32767)) && verbose)
            if ((keycount & (keycount-1))==0) {
                printcurrtime();
                printf("Got %lld keys\n", keycount);
            }
    }
};




template <typename KVpair>
class BinaryKmerReader: public KmerReader<KVpair> {
    FILE * f;
    static const int buflen = 8192;
    KVpair buff[buflen*2];
    int curr = 0;
    int max = 0;
    bool isclosed =false;
public:
    BinaryKmerReader(const char * fname) {
        char buf[1024];
        strcpy(buf,fname);
        if (buf[strlen(buf)-1]=='\n')
            buf[strlen(buf)-1] = '\0';
        f=fopen(buf,"rb");
        printf("OpenFile to read Kmers %s\n",fname);
        if (f==0) {
            printf("errno %d %s\n",errno,strerror(errno));
        }
        curr = 0;
        memset(buff,0,sizeof(buff));
        max = fread(buff,sizeof(buff[0]),buflen,f);
    }
    void finish() {
        if (!isclosed) {
            fclose(f);
        }
        isclosed = true;
    }
    ~BinaryKmerReader() {
        finish();
    }
    bool getNext(KVpair *ret) {
        if (max ==0) return false;
        if (curr == max) {
            max = fread(buff,sizeof(buff[0]),buflen,f);
            memset(ret,0xFF,sizeof(KVpair));
            if (max == 0) return false;
            curr = 0;
        }
        memcpy(ret, &buff[curr], sizeof(buff[curr]));
        curr++;
        return true;
    }
    void reset() {
        printf("Reset reading binary file. \n");
        rewind(f);
        curr = 0;
        memset(buff,0,sizeof(buff));
        max = fread(buff,sizeof(buff[0]),buflen,f);
    }
};


template <typename KVpair>
class BinaryKmerWriter {
    FILE *f;
    int curr = 0;
public:
    KVpair buf[1024];
    BinaryKmerWriter( const char * fname) {
        char buf[1024];
        strcpy(buf,fname);
        if (buf[strlen(buf)-1]=='\n')
            buf[strlen(buf)-1] = '\0';
        f=fopen(buf,"wb");
        printf("OpenFile to write Kmers %s %x\n",fname,f);
        curr = 0;
        memset(buf,0,sizeof(buf));
    }
    static const int buflen = 16;
    void write(KVpair *p) {
        memcpy(&buf[curr],p,sizeof(buf[curr]));
        curr++;
        if (curr == buflen) {
            fwrite(buf,sizeof(buf[0]),buflen,f);
            curr = 0;
        }
    }
    void finish() {
        fwrite(buf,sizeof(buf[0]),curr,f);
        curr = 0;
        fclose(f);
    }
};

//! read kmer from unsorted txt file and sort .
template <typename keyType>
class SortedKmerTxtReader : public KmerReader<keyType> {
    BinaryKmerReader<keyType> * binaryReader = NULL;
    uint32_t pointer;
    vector<keyType> * vK;
public:
    SortedKmerTxtReader(const char * fname, uint32_t kmerlength, const char *tmpfilename) {
        ConstantLengthKmerHelper<keyType, uint64_t> helper(kmerlength, 0);
        FileReader<keyType,uint64_t> *reader;
        reader = new KmerFileReader<keyType,uint64_t>(fname, &helper, false);
        keyType k;
        uint64_t v;
        vK = new vector<keyType>();
        while (reader->getNext(&k, &v)) {
            vK->push_back(k);
        }
        sort(vK->begin(),vK->end());
        delete reader;
        if (tmpfilename != NULL) {
            string binaryfilename (tmpfilename);
            BinaryKmerWriter<keyType> writer(binaryfilename.c_str());
            for (uint64_t k:*vK)
                writer.write(&k);
            writer.finish();
            binaryReader = new BinaryKmerReader<keyType> (binaryfilename.c_str());
        }
        else pointer = 0;
    }
    ~SortedKmerTxtReader() {
        finish();
        if (binaryReader)
            delete binaryReader;
    }
    bool getNext(keyType *k) {
        if (binaryReader!= NULL)
            return binaryReader->getNext(k);
        else {
            if (pointer == vK->size()) {
                delete vK;
                return false;
            }
            *k = (*vK)[pointer++];
            return true;
        }
    }
    void reset() {
        if (binaryReader!=NULL)
            binaryReader->reset();
        pointer = 0;

    }
    void finish() {
        if (binaryReader)
            binaryReader->finish();
    }
};


template <typename keyType, typename valueType>
class taxoTreeBuilder: public FileReader <keyType, valueType> {
    vector< FILE *> fV;
    vector<compressFileReader <keyType, valueType> *> readerV;
    vector< vector< int > > NCBI; //texonomyToNCBIID;
    vector< int > stID;
    struct KIDpair {
        keyType k;
        uint32_t id;
        bool finished;
        bool friend operator <( const KIDpair &a, const KIDpair &b) {
            if (a.finished != b.finished) return (((int) a.finished) > ((int) b.finished));
            return a.k>b.k;
        }
    };
public:
    void finish() {
        for (auto f: fV) fclose(f);
    }
    void reset() {
        printf(" Do not support reset() \n");
    }
    int levelcount;
    vector<vector<int> > NCBI_local;
    vector<int> localshift;
    vector<vector<string> > NCBI_ID;
    vector<KmerReader<uint64_t> *> readers;
    vector<MultivalueFileReaderWriter<uint64_t, uint16_t> *> grpreaders; //must be 64-bit kmers, and 16-bit grpids.
    priority_queue<KIDpair> PQ;
    bool combineMode = false; //used when there are >=800 files;
    uint32_t combineCount; // split the file into combineCount groups,
    bool getFileIsSorted() {
        return true;
    }
    void groupFile(string fname, vector<string> lf, string prefix, string suffix, int32_t idshift, bool useBinaryKmerFile,uint32_t KmerLength, const char * tmpfolder) {
        vector<KmerReader<keyType> *> readers;
        priority_queue<KIDpair> PQN;
        for (string s: lf) {
            string fname = prefix + s + suffix;
            if (useBinaryKmerFile)
                readers.push_back(new BinaryKmerReader<keyType>(fname.c_str()));
            else {
                string tmpfname(tmpfolder);
                tmpfname = tmpfname + s + ".bintmp";
                readers.push_back(new SortedKmerTxtReader<keyType>(fname.c_str(),KmerLength,NULL));
            }
            keyType key;
            readers[readers.size()-1]->getNext(&key);
            KIDpair kid = {key, idshift+readers.size()-1, false};
            PQN.push(kid);
        }

        MultivalueFileReaderWriter<keyType,uint16_t> * writer = new MultivalueFileReaderWriter<keyType,uint16_t> (fname.c_str(),8,2,false);
        // Loop key for these files;
        while (true) {
            keyType key = PQN.top().k;
            uint32_t id = PQN.top().id;
            vector<uint16_t> ret;
            if (PQN.top().finished) {
                for (auto r: readers) {
                    r->finish();
                    delete r;
                }
                writer->finish();
                delete writer;
                return;
            }
            while (PQN.top().k == key && !PQN.top().finished) {
                int tid = PQN.top().id;
                ret.push_back(tid);
                keyType nextk;
                bool finish = !readers[tid-idshift]->getNext(&nextk);
                PQN.pop();
                KIDpair kid = {nextk, tid, finish};
                PQN.push(kid);
            }
            writer->write(&key, ret);
        }
    }
    vector< vector<uint16_t> > grpTmpValue;

    taxoTreeBuilder(const char * NCBIfname, const char * fnameprefix, const char * fnamesuffix, const char * tmpFileDirectory, uint32_t KmerLength, uint32_t splitbit, bool useBinaryKmerFile = true ) {
        FileReader<keyType,valueType>::helper = new ConstantLengthKmerHelper<keyType,valueType> (KmerLength,splitbit);
        FILE * fNCBI;
        string prefix ( fnameprefix);
        string suffix (fnamesuffix);
        fNCBI = fopen(NCBIfname, "r");
        //Assuming the file is tab-splited,
        //Species_index	Species_ID	Species_name	Genus_index	Genus_ID	Genus_name	Family_index	Family_ID	Family_name	Order_index	Order_ID	Order_name	Class_index	Class_ID	Class_name	Phylum_index	Phylum_ID	Phylum_name
        char buf[4096];
        fgets(buf, 4096, fNCBI); // skip the first line
        vector<string> vv = split(buf, '\t');
        levelcount = vv.size()/3;
        NCBI_local.clear();
        NCBI_ID.clear();
        NCBI_local.resize(levelcount);
        NCBI_ID.resize(levelcount);
        readers.clear();
        vector<string> fnames;
        while (true) {
            if (fgets(buf, 4096, fNCBI) == NULL) break; // read a Species
            vector<string> vv = split(buf, '\t');
            if (vv.size()<2) break;
            for (int i = 0 ; i*3 < vv.size(); i++) {
                int localID = atoi(vv[i*3].c_str());
                NCBI_local[i].push_back(localID);
                NCBI_ID[i].push_back(vv[i*3+1]);
            }
            fnames.push_back(vv[1]);
        }
        localshift.clear();
        localshift.push_back(1);
        for (int i = 0; i < levelcount; i++)
            localshift.push_back(localshift[i] + *max_element(NCBI_local[i].begin(), NCBI_local[i].end())+1);

        int nn = 50;
        combineMode = (fnames.size()>nn);
        if (combineMode) {
            int curr = 0;
            int combineCount = 0;
            vector<string> * fnamesInThisgrp ;
            vector<string> grpfnames;
            while (curr < fnames.size()) {
                if (curr + nn < fnames.size())
                    fnamesInThisgrp = new vector<string> (fnames.begin()+curr, fnames.begin()+curr+nn);
                else
                    fnamesInThisgrp = new vector<string> (fnames.begin()+curr, fnames.end());
                stringstream ss;
                string tmpFolder(tmpFileDirectory);

                ss<<tmpFolder<<"TMP"<<grpfnames.size();
                string fnamegrp;
                ss>> fnamegrp;
                grpfnames.push_back(fnamegrp);
                printf("merge kmer files %d %d to grp %s\n", curr, curr+fnamesInThisgrp->size()-1, fnamegrp.c_str());
                groupFile(fnamegrp, *fnamesInThisgrp, prefix, suffix, curr, useBinaryKmerFile,KmerLength,tmpFileDirectory);
                curr += fnamesInThisgrp->size();
                delete fnamesInThisgrp;
            }
            combineCount = grpfnames.size();
            for (string v: grpfnames) {
                grpreaders.push_back( new MultivalueFileReaderWriter<uint64_t, uint16_t>(v.c_str(), 8,2, true));
                keyType key;
                uint16_t valuebuf[1024];
                grpreaders[grpreaders.size()-1]->getNext(&key, valuebuf);
                vector<uint16_t> Vvaluebuf;
                for (int i = 0 ; grpreaders[0]->valid(valuebuf[i]); i++)
                    Vvaluebuf.push_back(valuebuf[i]);
                grpTmpValue.push_back(Vvaluebuf);
                KIDpair kid = {key, grpreaders.size()-1, false};
                PQ.push(kid);
            }
        }
        else
            for (int i = 0 ; i < NCBI_ID.size(); i++) {
                string fname = prefix + fnames[i] + suffix;
                if (useBinaryKmerFile)
                    readers.push_back(new BinaryKmerReader<keyType>(fname.c_str()));
                else {
                    string tmpfname(tmpFileDirectory);
                    tmpfname = tmpfname + fnames[i] + ".bintmp";
                    readers.push_back(new SortedKmerTxtReader<keyType>(fname.c_str(),KmerLength,tmpfname.c_str()));
                }
                keyType key;
                readers[readers.size()-1]->getNext(&key);
                KIDpair kid = {key, readers.size()-1, false};
                PQ.push(kid);
            }
        fclose(fNCBI);
        string IDLfname(tmpFileDirectory);
        IDLfname+= "IDList.txt";
        FILE * IDLf;
        IDLf = fopen(IDLfname.c_str(),"w");
        for (int t : localshift) {
            fprintf(IDLf,"%d\n",t);
        }
        fclose(IDLf);
    }
    ~taxoTreeBuilder() {
        if (combineMode)  {
            for (int i = 0 ; i < grpreaders.size(); i++)
                delete grpreaders[i];
        }
        else
            for (int i = 0 ; i < readers.size(); i++)
                delete readers[i];
        delete FileReader<keyType,valueType>::helper;
    }
    bool getNext( keyType *k, valueType *v) {
        int anslevel = 0;
        keyType key = PQ.top().k;
        vector<int> ret;
        if (PQ.top().finished) {
            finish();
            return false;
        }
        // printf("Find key %llx:", key);
        while (PQ.top().k == key && !PQ.top().finished) {
            int tid;
            tid = PQ.top().id;
            keyType nextk;
            bool finish;
            if (combineMode) {
                ret.insert(ret.end(),grpTmpValue[tid].begin(),grpTmpValue[tid].end());
                int ll = grpTmpValue[tid].size();
                //       printf("   %d keys: (from %d)\t", ll, tid);
                //     for (int i: grpTmpValue[tid])
                //          printf("%x\t",i);
                uint16_t valuebuf[1024];
                finish = !grpreaders[tid]->getNext(&nextk, valuebuf);
                grpTmpValue[tid].clear();
                for (int i = 0; grpreaders[tid]->valid(valuebuf[i]); i++)
                    grpTmpValue[tid].push_back(valuebuf[i]);
                //    printf("Next Has ::%d::", grpTmpValue[tid].size());
            }
            else {
                ret.push_back(tid);
                //  printf(" %x\t",PQ.top().id);
                finish = !readers[tid]->getNext(&nextk);
            }
            PQ.pop();
            KIDpair kid = {nextk, tid, finish};
            PQ.push(kid);
        }
        *k = key;

        for (int i = 0; i< levelcount; i++) {
            bool flag = true;
            for (int j = 0; j < ret.size() && flag; j++)
                flag = (NCBI_local[i][ret[j]]==NCBI_local[i][ret[0]]);
            if (flag) {
                *v = localshift[i] + NCBI_local[i][ret[0]];
                return true;
            }
        }
        *v = localshift[levelcount];
        return true;
    }
};



template<typename keyType, typename valueType>
class VectorReader : public FileReader<keyType, valueType> {
public:
    vector<keyType> kV;
    vector<valueType> vV;
    bool fIsSorted;
    int cur = 0;
    VectorReader(IOHelper<keyType,valueType> *_helper, bool b, vector<keyType> &k, vector<valueType> &v) : kV(k),vV(v)  {
        fIsSorted = b;
        FileReader<keyType,valueType>::helper = _helper;
        cur = 0;
    }
    void finish() {
        kV.clear();
        vV.clear();
    }
    void reset() {
        cur = 0;
    }
    bool getFileIsSorted() {
        return fIsSorted;
    }
    ~ VectorReader () {
        finish();
    }
    bool getNext(keyType *T, valueType *V) {
        if (cur >= kV.size()) return false;
        *T = kV[cur];
        *V = vV[cur];
        cur++;
        return true;
    }

};

template<typename keyType, typename valueType>
class SortedVectorRefReader : public FileReader<keyType,valueType> {
public:
    vector<keyType> *kV;
    vector<valueType> *vV;
    int cur = 0;
    SortedVectorRefReader(vector<keyType> *k, vector<valueType> *v,IOHelper<keyType, valueType> * _helper
                         ) : kV(k),vV(v)  {
        FileReader<keyType,valueType>::helper = _helper;
        cur = 0;
    }
    void finish() {
    }
    void reset() {
        cur = 0;
    }
    bool getFileIsSorted() {
        return true;
    }
    ~ SortedVectorRefReader () {
        finish();
    }
    bool getNext(keyType *T, valueType *V) {
        if (cur >= kV->size()) return false;
        *T = (*kV)[cur];
        *V = (*vV)[cur];
        cur++;
        return true;
    }


};


class ValueConverter {
public:
    virtual uint8_t convert(uint32_t)  = 0;
};

class expressionConverter : ValueConverter {
    vector<uint32_t> v;
public:
    expressionConverter( vector<uint32_t> V): v(V) {

    }
    uint8_t convert(uint32_t k) {
        for (uint8_t ans= v.size() - 1; ans>0; ans--)
            if (k >= v[ans]) return ans;
        return 0;
    }
};
template <typename keyType, typename valueType>
class SinglevalueFileReaderWriter : public MultivalueFileReaderWriter< keyType, valueType> {
public:
    ValueConverter * converter;
    SinglevalueFileReaderWriter( const char * fname, bool _isRead = true, ValueConverter * _converter = NULL):
        MultivalueFileReaderWriter<keyType,valueType>(fname, sizeof(keyType), sizeof(valueType), _isRead)
    {
        converter = _converter;
    }
    ~SinglevalueFileReaderWriter() {
        MultivalueFileReaderWriter<keyType, valueType>::finish();
    }
    bool getNext(keyType *k, valueType *v) override {
        if (!MultivalueFileReaderWriter<keyType,valueType>::get(k,sizeof(keyType))) return false;
        MultivalueFileReaderWriter<keyType,valueType>::get(v,(sizeof(valueType)));
        if (converter != NULL)
            *v = converter->convert(*v);
        return true;
    }
    bool write(keyType *k ,valueType *v) override {
        MultivalueFileReaderWriter<keyType, valueType>::
        add((void *) k, sizeof(keyType));
        MultivalueFileReaderWriter<keyType, valueType>::
        add((void *) v, sizeof(valueType));
        return true;
    }
};

class ValuelistExpressionWriter : MultivalueFileReaderWriter<uint64_t, uint32_t> {
public:
    ValuelistExpressionWriter(const char * fname):
        MultivalueFileReaderWriter<uint64_t, uint32_t>(fname, sizeof(uint64_t), sizeof(uint32_t), false) {}
    ~ValuelistExpressionWriter() {
        MultivalueFileReaderWriter<uint64_t, uint32_t>::finish();
    }
    bool write(uint64_t &k, vector<pair<uint32_t, uint32_t>> &vec) {
        MultivalueFileReaderWriter<uint64_t, uint32_t>::
        add((void *) &k, sizeof(uint64_t));
        for (auto &p: vec) {
            MultivalueFileReaderWriter<uint64_t, uint32_t>::
            add((void *) &p.first, sizeof(uint32_t));
            MultivalueFileReaderWriter<uint64_t, uint32_t>::
            add((void *) &p.second, sizeof(uint32_t));
        }
        uint32_t pend= 0xFFFFFFFF;
        MultivalueFileReaderWriter<uint64_t, uint32_t>::
        add((void *) &pend, sizeof(uint32_t));
        return true;
    }
};


class ValuelistExpressionReader : MultivalueFileReaderWriter<uint64_t, uint32_t> {
public:
    ValuelistExpressionReader(const char * fname):
        MultivalueFileReaderWriter<uint64_t, uint32_t>(fname, sizeof(uint64_t), sizeof(uint32_t), true) {}
    ~ValuelistExpressionReader() {
        MultivalueFileReaderWriter<uint64_t, uint32_t>::finish();
    }
    bool getNext(uint64_t *k,vector<pair<uint32_t,uint32_t>> *vec) {
        if (!MultivalueFileReaderWriter<uint64_t, uint32_t>::get(k,sizeof(uint64_t))) return false;
        vec->clear();
        while (true) {
            uint32_t v1, v2;
            if (!MultivalueFileReaderWriter<uint64_t, uint32_t>::get(&v1,sizeof(uint32_t))) return false;
            if (v1 > 0xF0000000) return true;
            if (!MultivalueFileReaderWriter<uint64_t, uint32_t>::get(&v2,sizeof(uint32_t))) return false;
            vec->push_back(make_pair(v1,v2));
        }
        return true;
    }
};

template <typename keyType, int NNL>
struct BinaryBitSet {
public:
    unsigned char m[NNL/8];
    keyType k;
#ifdef BINARYSETLOGS
    int last1 = -1, last2 = -1;
#endif
    static_assert(NNL % 8 == 0, "multiples of 8");
    unsigned int count() {
        uint64_t u64[NNL/64+1];
        memset(u64,0,sizeof(u64));
        memcpy(u64,m,NNL/8);
        int cnt = 0;
        for (int i = 0 ; i < NNL; i+= 64) {
            cnt += __builtin_popcountll(u64[i>>6]);
        }
        return cnt;
    }
    void setvalue(unsigned int t) {
        m[t>>3] |= (1<<(t&7));
#ifdef BINARYSETLOGS
        last2 = last1;
        last1 = t;
#endif
    }
    void setvalue(unsigned int id, unsigned int value,unsigned int VALUEBIT) {
        unsigned int idx = id*VALUEBIT/8;
        unsigned int shift = (id*VALUEBIT) & 0x7;
        m[idx] |= (value << shift);
    }
    void reset() {
        memset(m,0,NNL/8);
    }
    bool operator [] (const int t) {
        return (m[t>>3] >> (t&7) & 1);
    }
    void fprint(int slotbase = 1048576, FILE *fout = stdout) {
        /*
        if (count() < 5) {
            for (int i = 0 ;i < NNL; i++)
                if ((*this)[i]) printf("%d\t",i);
        }
        else if (count() > NNL-5) {
            for (int i = 0; i < NNL; i++)
                if (!(*this)[i]) printf("%d\t",-i);
        }
        else
        */
        for (int i = 0 ; i < NNL; i+=slotbase) {
            int ans = 0;
            for (int j  = 0; j < slotbase; j++)
                ans += ((*this)[i+j] << j);
            fprintf(fout,"%x", ans);
        }
        fprintf(fout,"\n");
    }
    void printbitset(bitset<NNL> &id, int slotbase = 1048576) {
        for (int i = 0 ; i < NNL; i+=slotbase) {
            int ans = 0;
            for (int j  = 0; j < slotbase; j++)
                ans += (id[i+j] << j);
            printf("%x", ans);
        }
        printf("\n");
    }
} __attribute__((packed));


template <typename keyType>
struct BinaryVarlenBitSet {
public:
    size_t NNL;
    vector<unsigned char> m;
    keyType k;
    BinaryVarlenBitSet(keyType _k, size_t _NNL) //NNL*8 bits
        : NNL(_NNL), m(_NNL,0), k(_k) {
    }
    BinaryVarlenBitSet(keyType _k, vector<unsigned char> _m, size_t _NNL):
        NNL(_NNL), m(_m), k(_k) {
    }
    void setvalue(unsigned int t) {
        m[t>>3] |= (1<<(t&7));
    }
    void setvalue(unsigned int id, unsigned int value,unsigned int VALUEBIT) {
        unsigned int idx = id*VALUEBIT/8;
        unsigned int shift = (id*VALUEBIT) & 0x7;
        m[idx] |= (value << shift);
    }
    void reset() {
        fill(m.begin(), m.end(),0);
    }
    bool operator [] (const int t) {
        return (m[t>>3] >> (t&7) & 1);
    }
    void fprint(int slotbase = 1048576, FILE *fout = stdout) {
        for (int i = 0 ; i < NNL; i+=slotbase) {
            int ans = 0;
            for (int j  = 0; j < slotbase; j++)
                ans += ((*this)[i+j] << j);
            fprintf(fout,"%x", ans);
        }
        fprintf(fout,"\n");
    }
} __attribute__((packed));

