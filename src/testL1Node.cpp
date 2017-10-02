#include <L1Node.hpp>
#include <cstdlib>
#include <cstdio>
#include <random>
#include <algorithm>

int main(int argc, char* argv [] ) {
    int n = 10485760;
    int thread = 0;
    sscanf(argv[1], "%d", &n);
    sscanf(argv[2], "%d", &thread);
    vector<uint64_t> k;
    vector<uint16_t> v;
    for (int i = 0 ; i < n ; i++) {
         k.push_back((rand()^(rand()<<12))&0xFFFFFFFULL);
         v.push_back(rand()%0xFFFF);
    }
    sort(k.begin(), k.end());
    L1Node * p = new L1Node(1048575*128*4, 14);
    for (int i = 0 ; i < n ; i++) {
            p->add(k[i], v[i]);
    }
    p->constructAndWrite(14, thread, "test");
    int splitbit = p->getsplitbit();
    printf("%d", splitbit);
    /*
    L1Node *q = new L1Node(0,14);
    q->setsplitbit(14, splitbit);
    q->loadFromFile("test");
    int uneq = 0;
    for (int i = 0 ; i < n; i++) {
        uint64_t res = q->queryInt(k[i]);
        if ((res ^ v[i])& 0xFFF) {
                printf("query result for %lx : %lx, %x", k[i],  res,  v[i]);
        uneq++;
        }
    }
*/
}
/*
void testVAL(vector<uint32_t> val) {

        vector<uint8_t> buf(64,0);
        vector<uint8_t> buf0(64,0);
        uint32_t q = valuelistEncode(&buf[0], val, false);
        EXPECT_EQ(buf,buf0);
        uint32_t q2 = valuelistEncode(&buf[0], val, true);
        EXPECT_EQ(q,q2);
        vector<uint32_t> valret;
        uint32_t ql = valuelistDecode(&buf[0], valret, 64);
        EXPECT_EQ(ql, valret.size());
        EXPECT_EQ(valret, val); 
        
}
TEST_F(L2NodeTest, TestEncodeDecode) {
    for (int i = 0 ; i < 100; i++) {
        int l = 1+ i % 6;
        vector<uint32_t> val;
        for (int j = 0 ; j < l ; j++) {
               int x = rand() % (0xF);
               if (rand() & 1) x = ((x<<4) | (rand () % 0xF));
               if (rand() & 1) x = ((x<<4) | (rand () % 0xF));
               if (rand() & 1) x = ((x<<4) | (rand () % 0xF));
               if (rand() & 1) x = ((x<<4) | (rand () % 0xF));
               val.push_back(x+1);
        }
        testVAL(val);
   }
   vector<uint32_t> val1 = { 57, 13758, 5 };
   testVAL(val1);
}

TEST_F(L2NodeTest, TestL2Short) {
    
    L2Node *N = new L2ShortValueListNode (5,8);
    int NN = 20;
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, 0x6FFFFFFFULL);
    vector<uint64_t> vK; 
    for (uint64_t i=0; i<NN; i++) {
        uint64_t tmp = dis(gen);
        vK.push_back(tmp  ^ (tmp<<16));
        uint64_t k = vK[i];

        vector<uint32_t> v;
        for (int j = 1; j<=5; j++) v.push_back((i+j*37)%100);
        sort(v.begin(), v.end());
        N->add(k,v);
    }
    N->constructOth();

    for (uint64_t i=0; i<NN; i++) {
        uint64_t k = vK[i];
        vector<uint32_t> v,vret;
        vector<uint8_t> vretmap;
        for (int j = 1; j<=5; j++) v.push_back((i+j*37)%100);
        sort(v.begin(), v.end());

        bool ret = N->smartQuery(&k, vret, vretmap);
        EXPECT_EQ(ret, true);
        EXPECT_EQ(vret, v);
    }

    N->gzfname = "test.gz";
    N->writeDataToGzipFile();

    L2Node *N2 = new L2ShortValueListNode (5,8);
    N2->gzfname = "test.gz";
    N2->loadDataFromGzipFile();
    for (uint64_t i = 0; i < NN; i++) {
        uint64_t k = vK[i];
        vector<uint32_t> v,vret;
        vector<uint8_t> vretmap;
        for (int j = 1; j<=5; j++) v.push_back((i+j*37)%100);
        sort(v.begin(), v.end());
        bool ret = N2->smartQuery(&k, vret, vretmap);

        EXPECT_EQ(ret, true);
        EXPECT_EQ(vret, v);
    }
}

TEST_F(L2NodeTest, TestL2MAPP) {
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, 255);
    std::uniform_int_distribution<> dis2(0, 0x6FFFFFFFULL);
	int totN = 101;
	int totK = 256;
    int L = 12;
	vector<uint8_t> buf;
    vector<uint64_t> vK;
	for (int i = 0; i <= totN*L; i++) {
		buf.push_back(dis(gen));
	}
    for (uint64_t i=0; i<totN; i++) {
        uint64_t tmp = dis2(gen);
        vK.push_back(tmp  ^ (tmp<<16));
    }
	L2Node *N = new L2EncodedValueListNode (L,L2NodeTypes::MAPP);
    for (uint64_t i=0; i<totN; i++) {
        uint64_t k = vK[i];
		vector<uint8_t> tmp(buf.begin()+(i*L), buf.begin() + ((i+1)*L));
        N->addMAPP(k,tmp);
    }
    N->constructOth();

    N->gzfname = "test.gz";
    N->writeDataToGzipFile();

    gzFile fin = gzopen("test.gz", "rb");
    L2Node *N2 = new L2ShortValueListNode (4,6);
    N2->gzfname = "test.gz";
    N2->loadDataFromGzipFile();

    for (uint64_t i=0; i<totN; i++) {
        uint64_t k = vK[i];
        vector<uint32_t> vret;
        vector<uint8_t> vretmap, vretmap2;
		vector<uint8_t> tmp(buf.begin()+(i*L), buf.begin() + ((i+1)*L));
        bool ret = N->smartQuery(&k, vret, vretmap);
        bool ret2 = N->smartQuery(&k, vret, vretmap2);
        EXPECT_EQ(ret, false);
        EXPECT_EQ(ret2, false); 
		EXPECT_EQ(vretmap, tmp);
		EXPECT_EQ(vretmap2, tmp);
    }
}

TEST_F(L2NodeTest, TestL2EncodeLong) {
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, 255);
    std::uniform_int_distribution<> dis2(0, 0x6FFFFFFFULL);
	int totN = 101;
	int totK = 256;
    int L = 12;
	vector<vector<uint32_t>> vlists;
    vector<uint64_t> vK;
    uint32_t maxlength = 0;
	for (int i = 0; i < totN; i++) {
       vector<uint32_t> vec;
       uint32_t last;
       vec.push_back(last = dis(gen));
       uint32_t upper = dis2(gen) % 15 + 1;
       for (int i = 1 ; i<= upper; i++) {
           last += (dis(gen) + 1);
           if (dis2(gen) & 1) 
               last += dis(gen);
           if (dis2(gen) & 3) 
               last += dis(gen);
           vec.push_back(last);
       }
       vlists.push_back(vec);
       vector<uint32_t> diff; diff.push_back(vec[0]);
       for (int i = 1; i< vec.size(); i++)
           diff.push_back(vec[i] - vec[i-1]);
       uint32_t encodelength = valuelistEncode(NULL, diff, false);
       if (encodelength > maxlength) maxlength = encodelength + 1;
       
    }
    for (uint64_t i=0; i<totN; i++) {
        uint64_t tmp = dis2(gen);
        vK.push_back(tmp  ^ (tmp<<16));
    }

	L2Node *N = new L2EncodedValueListNode (maxlength,L2NodeTypes::VALUE_INDEX_ENCODED);
    for (uint64_t i=0; i<totN; i++) {
        uint64_t k = vK[i];
		vector<uint32_t> vec = vlists[i];
        vector<uint32_t> diff; diff.push_back(vec[0]);
        for (int i = 1; i< vec.size(); i++)
           diff.push_back(vec[i] - vec[i-1]);
         N->add(k,diff);
    }
    N->constructOth();

    gzFile fout = gzopen("test.gz", "wb");
    N->gzfname = "test.gz";
    N->writeDataToGzipFile();
    gzclose(fout);
    gzFile fin = gzopen("test.gz", "rb");
	L2Node *N2 = new L2EncodedValueListNode (maxlength,L2NodeTypes::VALUE_INDEX_ENCODED);
    N2->gzfname = "test.gz";
    N2->loadDataFromGzipFile();

    for (uint64_t i=0; i<totN; i++) {
        uint64_t k = vK[i];
        vector<uint32_t> vret, vret2;
        vector<uint8_t> vretmap, vretmap2;
        bool ret = N->smartQuery(&k, vret, vretmap);
        bool ret2 = N->smartQuery(&k, vret2, vretmap2);
        EXPECT_EQ(ret, true);
        EXPECT_EQ(ret2, true); 
        vector<uint32_t> vl = vlists[i];
		EXPECT_EQ(vret, vl);
		EXPECT_EQ(vret2, vl);
    }

}
*/

