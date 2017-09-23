#pragma once
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include "io_helper.hpp"
using namespace std;
struct KV6432 {
    uint64_t k;
    uint32_t v;
    bool friend operator <( const KV6432 &a, const KV6432 &b) {
        return a.k < b.k;
    }
} __attribute__((packed));


template<typename keyType, typename valueType, class fileReader> //filereader supports ->get(k,v)
//this->getNext.. gets vector<pair<id,v>>
class SGroupReader {
    struct KIDpair {
        keyType k;
        valueType v;
        uint32_t id;
        bool finished;
        bool friend operator <( const KIDpair &a, const KIDpair &b) {
            if (a.finished != b.finished) return (((int) a.finished) > ((int) b.finished));
            return a.k>b.k;
        }
    };
    vector<fileReader *> readers;
    priority_queue<KIDpair> PQ;
protected:
    bool hasNext = true;

public:
    SGroupReader() {}
    SGroupReader(vector<string> & fnames) {
        for (auto const fname : fnames) {
            readers.push_back(new fileReader(fname.c_str()));
            keyType k;
            valueType v;
            readers[readers.size()-1]->getNext(&k, &v);
            KIDpair kid = {k, v, (uint32_t) (readers.size()-1), false};
            PQ.push(kid);
        }
    }

    ~SGroupReader() {
        for (int i = 0 ; i < readers.size(); i++)
            delete readers[i];
    }
    unsigned long long keycount = 0;
    unsigned long long matchcount = 0;
public:
    bool getNextValueList(keyType &k, vector<pair<uint32_t, valueType> > &ret) {
        k = PQ.top().k;
        if (PQ.top().finished) {
            return false;
        }
        ret.clear();
        while (PQ.top().k == k && !PQ.top().finished) {
            int tid;
            tid = PQ.top().id;
            keyType nextk;
            valueType nextv;
            ret.push_back(make_pair(tid,PQ.top().v));
            bool finish = !readers[tid]->getNext(&nextk, &nextv);
            PQ.pop();
            KIDpair kid = {nextk, nextv, (uint32_t) tid, finish};
            PQ.push(kid);
        }
        updatekeycount();
        return true;
    }

protected:
    virtual void updatekeycount() {
        keycount ++;
        if (keycount > 1000000)
            if ((keycount & (keycount-1))==0) {
                printcurrtime();
                printf("Got %lld keys\n", keycount);
            }
    }
};

template <typename keyType> 
class GrpReader {
public:
     std::shared_ptr<
         SGroupReader<
            uint64_t, //key
            vector<pair<uint32_t, uint32_t>>, //each value is a list of ID/value
            ValuelistExpressionReader
            > > reader;
     int  fnamecnt;
     int maxlimit = 0x7FFFFFFF;
    GrpReader(string fname, string folder, int _maxlimit = -1) {
        if (_maxlimit >=0)
            maxlimit = _maxlimit;

        FILE * fNCBI;
        fNCBI = fopen(fname.c_str(), "r");
        char buf[4096];
        vector<string> fnames;
        while (true) {
            if (fgets(buf, 4096, fNCBI) == NULL) break;
            string fname(buf);
            if (*fname.rbegin() == '\n') 
                fname = fname.substr(0,fname.size()-1);
            fnames.push_back(folder+fname);
        }
        reader = make_shared<     SGroupReader<
            uint64_t, //key
            vector<pair<uint32_t, uint32_t>>, //each value is a list of ID/value
            ValuelistExpressionReader
            >>(fnames);
        fnamecnt = fnames.size();
    }
    bool getNextValueList(
       keyType &k,
       vector< pair<uint32_t, uint32_t> > & ans)
       {
         vector<pair<uint32_t,vector<pair<uint32_t,uint32_t>> >> ret; //return is a list of pair<ID, V>, where V is a vector<p<32,32>>
         bool res = reader->getNextValueList(k, ret);
         if (res) {
            ans.clear();
            for (auto const &v: ret) {
                ans.insert(ans.end(), v.second.begin(), v.second.end());
            }
         }
         return res & (maxlimit-- > 0);
    }
};

int encodelengths(const vector<uint32_t> &data, int &bestk) {
    //encode this as:
    //4 bits: k (max = 16, min=2)
    //next k*n bits: 
    // encode the 'gaps': x0,x1,x2....:
    //             index of 1's: x0, x0+x1+1, x0+x1+x2+2, ...
    //             range of x_i: >=0
    //             encoded as: (say k=4),
    //                         0..0xE: 0..14
    //                         15t+14: t times of F, and a cell of 0..E
    //             at the end: all F.
    
    vector<int> cnt(17,0);
    for (int i = 0 ; i < data.size(); i++) {
       int x;
       if (i ==0) x= data[i]; else x=data[i]-data[i-1]-1;
       for (int p = 2; p<=16; p++) {
           cnt[p] += (x / ((1<<p) -1));
       }
    }
    int ans = 0x7FFFF;
    const vector<int> candi({2,4,8,12});
    for (int k : candi) 
        if (4 + k*(data.size() + cnt[k]) < ans) {
           ans = 4+ k*(data.size() + cnt[k]);
           bestk = k;
        }
    return ans;
}
