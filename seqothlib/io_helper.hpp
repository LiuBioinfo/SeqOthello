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
    keyType minSelfAndRevcomp(const keyType k) {
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
    //bool finished;
    bool friend operator <( const KIDpair &a, const KIDpair &b) {
        //if (a.finished != b.finished) return (((int) a.finished) > ((int) b.finished));
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
            bool finished = !readers[readers.size()-1]->getNext(&k);
            KIDpair<keyType> kid = {k, (uint32_t) (readers.size()-1)};
            if (!finished) PQ.push(kid);
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
        if (PQ.empty()) {
            return false;
        }
        ret.clear();
        while (PQ.top().k == k) {
            int tid;
            tid = PQ.top().id;
            keyType nextk;
            ret.push_back(tid);
            bool finish = !readers[tid]->getNext(&nextk);
            if (nextk <= k) {
                throw std::invalid_argument("error getting nextk!");
            }
            PQ.pop();
            if (!finish) {
                KIDpair<keyType> kid = {nextk, (uint32_t) tid};
                PQ.push(kid);
            }
            if (PQ.empty()) break;
        }
        updatekeycount();
        return true;
    }
protected:
    void updatekeycount() {
        keycount ++;
        if ((keycount & 0xFFFF) ==0 )
            if ((keycount & (keycount-1))==0) {
                printcurrtime();
                printf("Got %lu keys\n", keycount);
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
    static const int buflen = 8192*4;
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
        if (keycount > 1048576) if ((keycount & (keycount - 1)) ==0) {
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
        return true;
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
    vector<string> fnames;
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
    int32_t getKmerLength() {
        return kmerlength;
    }
    void putSampleInfoToXml(tinyxml2::XMLElement * p) {
        for (auto s:fnames) {
            string grpfname = s + ".xml";
            tinyxml2::XMLDocument doc;
            doc.LoadFile(grpfname.c_str());
            for (tinyxml2::XMLElement *q = doc.FirstChildElement("Root")->FirstChildElement("Samples")->FirstChildElement("SampleInfo"); q!= NULL; q=q->NextSiblingElement("SampleInfo")) {
                tinyxml2::XMLNode *cpyNode = q->DeepClone(p->GetDocument());
                p->InsertEndChild(cpyNode);
            }
        }
    }
    KmerGroupComposer(vector<string> & _fnames): fnames(_fnames) {

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
        for (unsigned int i = 0 ; i < fnames.size(); i++) {
            auto const fname = fnames[i];
            auto const fnamexml = fname + ".xml";
            tinyxml2::XMLDocument xml;
            printf("load xml info from %s\n", fnamexml.c_str());
            xml.LoadFile( fnamexml.c_str());
            int tmpint;
            xml.FirstChildElement("Root")->FirstChildElement("GroupInfo")->QueryIntAttribute("TotalSamples", &tmpint);
            int64_t tmpi64;
            xml.FirstChildElement("Root")->FirstChildElement("GroupInfo")->QueryInt64Attribute("Keycount", &tmpi64);
            totkeycount.push_back(tmpi64);
            tmpval[i].resize(tmpint+1);
            shift.push_back(tmpint + *shift.rbegin());
            readers.push_back(new MultivalueFileReaderWriter<uint64_t, uint8_t>(fname.c_str(), sizeof(uint64_t), sizeof(uint8_t), true));
            keyType k;
            readers[readers.size()-1]->getNext(&k, &tmpval[i][0]);
            KIDpair<keyType> kid = {k, (uint32_t) (readers.size()-1)};
            PQ.push(kid);
            readkeys.push_back(1);
        }
    }
    virtual ~KmerGroupComposer() {
        for (unsigned int i = 0 ; i < readers.size(); i++)
            delete readers[i];
    }

    bool verbose = false;
    uint64_t keycount = 0;
    virtual bool getNextValueList(keyType &k, vector<uint32_t> &ret) {
        if (PQ.empty()) {
            return false;
        }
        k = PQ.top().k;
        ret.clear();
        while (PQ.top().k == k) {
            int tid;
            tid = PQ.top().id;
            keyType nextk;
            readkeys[tid]++;
            for (int i = 0; readers[tid]->valid(tmpval[tid][i]); i++) {
                ret.push_back(shift[tid] + tmpval[tid][i]);
            }
            PQ.pop();
            bool finish = !readers[tid]->getNext(&nextk, &tmpval[tid][0]);
            if (!finish) {
                if (nextk <= k) {
                    throw std::invalid_argument("error getting nextk!");
                }
                KIDpair<keyType> kid = {nextk, (uint32_t) tid};
                PQ.push(kid);
            }
            if (PQ.empty()) break;
        }

        updatekeycount();
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

        for (uint32_t i = 0; i < readers.size(); i++) {
            readers[i]->getNext(&k, &tmpval[i][0]);
            KIDpair<keyType> kid = {k, i};
            PQ.push(kid);
            readkeys[i] = 1;
        }
    }
protected:
    void updatekeycount() {
        keycount ++;
        if (keycount > 100000 && verbose)
            if ((keycount & (keycount-1))==0) {
                printcurrtime();
                printf("Got %lu keys\n Bytes read from groups: ", keycount);
                for (unsigned int i = 0 ; i < readers.size(); i++)
                    printf("%d:%5lldM\t", i, readers[i]->getpos()/1048576);
                printf("\n");
            }
    }
};
/*
template <typename keyType>
class KmerGroupComposerGenerato  : KmerGroupComposer<keyType> {
        int nn, ll;
        uint64_t cur = 0, cnt = 0;
public:
      KmerGroupComposerGenerator(int n, int l): nn(n), ll(l), KmerGroupComposer<keyType>(vector<string>()){}
      virtual ~KmerGroupComposerGenerator() {}
      bool getNextValueList(keyType &k, vector<uint32_t> &ret) {
           cnt ++;
           if (cnt >= nn) return false;
           cur += random()%1024;
           ret.clear();
           for (int i = 0 ; i < ll; i++)
                 ret.push_back(i);
           return true;
      }

};
*/



template <typename KVpair>
class BinaryKmerReader: public KmerReader<KVpair> {
    FILE * f;
    static const int buflen = 16384;
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
    KVpair buf[4096];
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
    static const int buflen = 2048;
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
};
//__attribute__((packed));

template <typename keyType>
class IOBuf {
    vector<keyType> v;
    FILE * fout;
    size_t tot = 0;
    bool load = false;
    bool done = false;
    char fname[1024]; 
    void load_from_file() {
       if (!done) 
               finishwrite();
       v.resize(tot);
       FILE *fin = fopen(fname,"rb");
       size_t max = fread(&v[0], sizeof(keyType), tot, fin);
       if (max != tot) {
               fprintf(stderr,"Warning reading %s: read %d elements , expected %d elements .\n", fname, max, tot);
       }
       load = true;
       fclose(fin);
       if ( remove(fname) != 0) {
               fprintf(stderr,"faile to remove file %s\n", fname);
       }
    }
public:
    IOBuf(const char * _fname) {
        strcpy(fname,_fname);
        fout = fopen(fname,"wb");
        if (fout == NULL) {
              fprintf(stderr,"failed to open file %s\n", fname);
        }
        v.clear();
    }
    void finishwrite() {
        if (v.size())
            fwrite(&v[0], v.size(), sizeof(keyType), fout);
        fclose(fout);
        done = true;
        v.clear();
    }
    void push_back(const keyType &t) {
        tot ++; 
        v.push_back(t);
        if (v.size() == 8192) {
            fwrite(&v[0], v.size(), sizeof(keyType), fout);
            v.clear();
        }
    }
    
    size_t size() {
        return tot;
    }
    keyType * getstart() {
        if (!load) 
             load_from_file();
        return &v[0];
    }
};

