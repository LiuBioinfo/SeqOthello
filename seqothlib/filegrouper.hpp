// This file is a part of SeqOthello. Please refer to LICENSE.TXT for the LICENSE
/*!
 * \file filecombi.h
 * Contains IO utilities.
 */
#pragma once
#include <io_helper.hpp>



template <typename keyType, int NNL>
class KmerGroupedReader { //: public FileReader <keyType, unsigned char [NNL/8] > {
    vector< FILE *> fV;
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
    int fnamecnt;
private:
    bool detailmap = false; //when detailmap == false, the read only returns the key, not the bitmap.
    vector<KmerReader<keyType> *> readers;
    vector<MultivalueFileReaderWriter<keyType, uint16_t> *> grpreaders; //must use 16-bit grpids.
    priority_queue<KIDpair> PQ;
    bool combineMode = false; //used when there are >=800 files;
    uint32_t combineCount; // split the file into combineCount groups,
    bool getFileIsSorted() {
        return true;
    }
    KmerReader<keyType> *filter = NULL;
protected:
    keyType nextPossibleKey;
    bool hasNext = true;
    void setKmerFilter(KmerReader<keyType> * _reader) {
        filter = _reader;
        hasNext = filter->getNext(&nextPossibleKey);
    }

    void groupFile(string fname, vector<string> lf, string prefix,  int32_t idshift, bool useBinaryKmerFile, const char * tmpfolder) {
        vector<KmerReader<keyType> *> readers;
        priority_queue<KIDpair> PQN;
        for (string s: lf) {
            string fname = prefix + s ;
            if (useBinaryKmerFile)
                readers.push_back(new BinaryKmerReader<keyType>(fname.c_str()));
            else {
                string tmpfname(tmpfolder);
                tmpfname = tmpfname + s + ".bintmp";
                readers.push_back(new SortedKmerTxtReader<keyType>(fname.c_str(),KmerLength,NULL));
            }
            keyType key;
            readers[readers.size()-1]->getNext(&key);
            KIDpair kid = {key, (uint32_t) (idshift+readers.size()-1), false};
            PQN.push(kid);
        }

        MultivalueFileReaderWriter<keyType,uint16_t> * writer;
        writer = new MultivalueFileReaderWriter<keyType,uint16_t> (fname.c_str(),8,2,false,filter);


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
                printf("find %d keys\n", writer->getKeycount());
                writer->finish();
                delete writer;
                return;
            }
            while (PQN.top().k == key && !PQN.top().finished) {
                int tid = PQN.top().id;
                if (detailmap) ret.push_back(tid);
                keyType nextk;
                bool finish = !readers[tid-idshift]->getNext(&nextk);
                PQN.pop();
                KIDpair kid = {nextk, (uint32_t) tid, finish};
                PQN.push(kid);
            }
            writer->write(&key, ret);
        }
    }
    vector< vector<uint16_t> > grpTmpValue;

    ConstantLengthKmerHelper<keyType,uint8_t> *pubhelper;

public:
    uint32_t KmerLength;
    virtual int getFileCount() {
        return fnamecnt;
    }
    virtual void finish() {
        for (auto f: fV) fclose(f);
    }
    virtual void reset() {
        printf(" Do not support reset() \n");
    }
    KmerGroupedReader() {}
    KmerGroupedReader(const char * NCBIfname, const char * fnameprefix, const char * tmpFileDirectory, uint32_t _KmerLength, bool useBinaryKmerFile = true, bool _Detailmap = true, KmerReader<keyType> *_filter = NULL  ) {
        detailmap = _Detailmap;
        KmerLength = _KmerLength;
        ConstantLengthKmerHelper<keyType,uint8_t> helper(KmerLength,-1);
        pubhelper = new ConstantLengthKmerHelper<keyType,uint8_t>(KmerLength,-1);
        if (_filter!=NULL)
            setKmerFilter(_filter);
        FILE * fNCBI;
        string prefix ( fnameprefix);
        fNCBI = fopen(NCBIfname, "r");
        //Assuming each line of the file contains a filename. unless , if the first line is '=XX', then this file describes a list of intermediate files created during the construction., and there are XX samples. e.g, '=2560'
        char buf[4096];
        readers.clear();
        vector<string> fnames;
        while (true) {
            if (fgets(buf, 4096, fNCBI) == NULL) break; // read a Species
            string fname(buf);
            if (*fname.rbegin() == '\n') fname = fname.substr(0,fname.size()-1);
            fnames.push_back(fname);
        }
        bool combineFromIntermediate = (fnames[0][0] == '=');
        if (combineFromIntermediate) {
            char buf[1024];
            strcpy(buf, fnames[0].c_str());
            sscanf(buf+1,"%d",&fnamecnt);
        }
        else
            fnamecnt = fnames.size();
#ifndef FILE_PER_GRP
#define FILE_PER_GRP 500
#endif
        int nn = FILE_PER_GRP;
        combineMode = (fnames.size()>nn) || combineFromIntermediate;
        if (combineMode) {
            int curr = 0;
            int combineCount = 0;
            vector<string> grpfnames;
            if (combineFromIntermediate) {
                fnames.erase(fnames.begin());
                string tmpFolder(tmpFileDirectory);
                for (auto &s: fnames) {
                    grpfnames.push_back(tmpFolder+s);
                }
            }
            else
                while (curr < fnames.size()) {
                    vector<string> * fnamesInThisgrp ;
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
                    groupFile(fnamegrp, *fnamesInThisgrp, prefix,  curr, useBinaryKmerFile,tmpFileDirectory);
                    curr += fnamesInThisgrp->size();
                    delete fnamesInThisgrp;
                }
            combineCount = grpfnames.size();
            for (string v: grpfnames) {
                grpreaders.push_back( new MultivalueFileReaderWriter<keyType, uint16_t>(v.c_str(), sizeof(keyType),2, true));
                keyType key;
                uint16_t valuebuf[1024];
                grpreaders[grpreaders.size()-1]->getNext(&key, valuebuf);
                vector<uint16_t> Vvaluebuf;
                for (int i = 0 ; grpreaders[0]->valid(valuebuf[i]); i++)
                    Vvaluebuf.push_back(valuebuf[i]);
                grpTmpValue.push_back(Vvaluebuf);
                KIDpair kid = {key, (uint32_t) (grpreaders.size()-1), false};
                PQ.push(kid);
            }
        }
        else
            for (int i = 0 ; i < fnames.size(); i++) {
                string fname = prefix + fnames[i] ;
                if (useBinaryKmerFile)
                    readers.push_back(new BinaryKmerReader<keyType>(fname.c_str()));
                else {
                    string tmpfname(tmpFileDirectory);
                    tmpfname = tmpfname + fnames[i] + ".bintmp";
                    readers.push_back(new SortedKmerTxtReader<keyType>(fname.c_str(),KmerLength,tmpfname.c_str()));
                }
                keyType key;
                readers[readers.size()-1]->getNext(&key);
                KIDpair kid = {key, (uint32_t) (readers.size()-1), false};
                PQ.push(kid);
            }
        filter->reset();
        fclose(fNCBI);
    }

    ~KmerGroupedReader() {
        if (combineMode)  {
            for (int i = 0 ; i < grpreaders.size(); i++)
                delete grpreaders[i];
        }
        else
            for (int i = 0 ; i < readers.size(); i++)
                delete readers[i];
    }
    unsigned long long keycount = 0;
    unsigned long long matchcount = 0;
    virtual bool getNext( BinaryBitSet<keyType, NNL> *kvpair) {
        if (filter == NULL)
            return getNextImpl(kvpair);
        while (hasNext) {
            if (!getNextImpl(kvpair)) return hasNext = false;
            if (kvpair->k < nextPossibleKey)
                while ((kvpair->k < nextPossibleKey)) {
                    if (!getNextImpl(kvpair)) return false;
                }
            if (kvpair->k == nextPossibleKey) {
                hasNext = filter->getNext(&nextPossibleKey);
                matchcount++;
                return true;
            }
            while (hasNext && (kvpair->k > nextPossibleKey)) {
                hasNext = filter->getNext(&nextPossibleKey);
            }
            if (kvpair->k == nextPossibleKey) {
                if (hasNext)
                    hasNext = filter->getNext(&nextPossibleKey);
                matchcount++;
                return true;
            }
        }
        return false;
    }
private:

    template<typename valueT>
    bool getNextValueListImpl(keyType *k, vector<valueT> &ret) {
        *k = PQ.top().k;
        if (PQ.top().finished) {
            finish();
            return false;
        }
        ret.clear();
        while (PQ.top().k == *k && !PQ.top().finished) {
            int tid;
            tid = PQ.top().id;
            keyType nextk;
            bool finish;
            if (combineMode) {
                ret.insert(ret.end(),grpTmpValue[tid].begin(),grpTmpValue[tid].end());
                int ll = grpTmpValue[tid].size();
                uint16_t valuebuf[1024];
                finish = !grpreaders[tid]->getNext(&nextk, valuebuf);
                if (detailmap) {
                    grpTmpValue[tid].clear();
                    for (int i = 0; grpreaders[tid]->valid(valuebuf[i]); i++)
                        grpTmpValue[tid].push_back(valuebuf[i]);
                }
            }
            else {
                if (detailmap) ret.push_back(tid);
                finish = !readers[tid]->getNext(&nextk);
            }
            PQ.pop();
            KIDpair kid = {nextk, (uint32_t) tid, finish};
            PQ.push(kid);
        }
        updatekeycount();
        return true;
    }
public:
    virtual bool getNextImpl( BinaryBitSet<keyType, NNL> *kvpair)
    {
        vector<int> ret;
        if (!getNextValueListImpl( & kvpair->k, ret)) return false;
        kvpair -> reset();
        if (detailmap)
            for (auto a: ret) {
                kvpair->setvalue(a);
            }
        return true;
    }

    template<typename valueT>
    bool getNextValueList(keyType *k, vector<valueT> &v) {
        v.clear();
        if (filter == NULL)
            return getNextValueListImpl(k,v);
        while (hasNext) {
            if (!getNextValueListImpl(k,v)) return hasNext = false;
            if (*k < nextPossibleKey)
                while ((*k < nextPossibleKey)) {
                    if (!getNextValueListImpl(k,v)) return false;
                }
            if (*k == nextPossibleKey) {
                hasNext = filter->getNext(&nextPossibleKey);
                matchcount++;
                return true;
            }
            while (hasNext && (*k > nextPossibleKey)) {
                hasNext = filter->getNext(&nextPossibleKey);
            }
            if (*k == nextPossibleKey) {
                if (hasNext)
                    hasNext = filter->getNext(&nextPossibleKey);
                matchcount++;
                return true;
            }
        }

    }
protected:
    virtual void updatekeycount() {
        keycount ++;
        if (keycount > 1000000)
            if ((keycount & (keycount-1))==0) {
                printcurrtime();
                printf("Got %lld keys\n", keycount);
                if (filter!=NULL)
                    printf("Passed %lld keys\n", matchcount);
                if (combineMode) {
                    printf("Currpos:\t");
                    for (auto a: grpreaders)
                        printf("%lld\t", a->getpos());
                    printf("\n");
                }

            }
    }
};


template <typename keyType, int NNL, unsigned int VALUEBIT>
class KmerExpressionGroupedReader : KmerGroupedReader<keyType, NNL*VALUEBIT> {
    struct KVID3 {
        keyType k;
        uint16_t v;
        uint32_t id;
        bool finished;
        bool friend operator <( const KVID3 &a, const KVID3 &b) {
            if (a.finished != b.finished) return (((int) a.finished) > ((int) b.finished));
            return a.k>b.k;
        }
    };
    int keycount = 0;
    priority_queue<KVID3> PQ;
    vector<SinglevalueFileReaderWriter<keyType,uint16_t> *> readers;
//    combinemode not suported yet.
public:
    KmerExpressionGroupedReader(const char * NCBIfname, const char * fnameprefix, const char * tmpFileDirectory, uint32_t _KmerLength, bool useBinaryKmerFile = true, ValueConverter * valueConverter = NULL, KmerReader<keyType> * _filter = NULL  ) {
        KmerGroupedReader<keyType,NNL*VALUEBIT>:: KmerLength = _KmerLength;
        ConstantLengthKmerHelper<keyType,uint8_t> helper(_KmerLength,-1);
        FILE * fNCBI;
        string prefix ( fnameprefix);
        fNCBI = fopen(NCBIfname, "r");
        //Assuming each line of the file contains a filename.
        char buf[4096];
        readers.clear();
        vector<string> fnames;
        if (_filter!=NULL)
            this->setKmerFilter(_filter);
        while (true) {
            if (fgets(buf, 4096, fNCBI) == NULL) break; // read a Species
            string fname(buf);
            fnames.push_back(fname);
        }
        KmerGroupedReader<keyType,NNL*VALUEBIT>::fnamecnt = fnames.size();
        if (fnames.size()> 500) {
            printf("Do not support >=500 names yet\n");
            return;
        }
        if (!useBinaryKmerFile) {
            printf("File type not supported. Please use 64-16 binary files");
            return;
        }
        for (int i = 0 ; i < fnames.size(); i++) {
            string fname = prefix + fnames[i] ;
            readers.push_back(new SinglevalueFileReaderWriter<keyType,uint16_t>(fname.c_str(),true, valueConverter));
            keyType key;
            uint16_t value;
            readers[readers.size()-1]->getNext(&key,&value);
            KVID3 kid = {key, value, (uint32_t) (readers.size()-1), false};
            PQ.push(kid);
        }
        fclose(fNCBI);
    }

    virtual bool getNextImpl( BinaryBitSet<uint64_t, NNL*VALUEBIT> *kvpair) override {
        keyType key = PQ.top().k;
        vector<pair<int,uint16_t>> ret;
        if (PQ.top().finished) {

            KmerGroupedReader<keyType,NNL*VALUEBIT>::finish();
            return false;
        }
        while (PQ.top().k == key && !PQ.top().finished) {
            int tid;
            tid = PQ.top().id;

            keyType nextk;
            bool finish;
            ret.push_back({tid,PQ.top().v});
            uint16_t nextv;
            finish = !readers[tid]->getNext(&nextk, &nextv);
            PQ.pop();
            KVID3 kid = {nextk, nextv, (uint32_t) tid, finish};
            PQ.push(kid);
        }
        kvpair -> reset();
        kvpair -> k = key;
        for (auto &a: ret) {
            if (VALUEBIT == 1)
                kvpair->setvalue(a.first);
            if (VALUEBIT > 1) {
                int id = (a.first);
                (*kvpair).setvalue(id, a.second & ((1<<VALUEBIT) - 1), VALUEBIT);
            }
        }
        KmerGroupedReader<keyType,NNL*VALUEBIT>:: updatekeycount();
        return true;
    }
};

template <typename keyType, int NNL>
class TestKmerGroupedReader : public KmerGroupedReader<keyType, NNL> { //: public FileReader <keyType, unsigned char [NNL/8] > {
public:
    int fncnt;
    int keyleft;
    TestKmerGroupedReader(int _fn, int _kl) {
        fncnt = _fn;
        keyleft = _kl;
    }
    int getFileCount() override {
        return fncnt;
    }
    void finish() override {}
    void reset() override {}
    int keycount = 0;
    bool getNext( BinaryBitSet<uint64_t, NNL> *kvpair) override {
        if (keyleft ==0) return false;
        kvpair -> reset();
        kvpair -> k = (((keyType) keyleft << 32) | keycount ^ 0x49218c3);
        int i = 1;
        uint32_t tmp = keycount^(keycount*keycount) ^ 0x18c83;
        kvpair -> setvalue(tmp % fncnt);
        keyleft --;
        keycount ++;
        if ((keycount & (keycount-1))==0) {
            printf("Got %lld keys\n", keycount);
        }
        return true;
    }
};

