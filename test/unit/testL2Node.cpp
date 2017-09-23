#include <L2Node.hpp>
#include "testL2Node.h"
#include <cstdlib>
#include <cstdio>

L2NodeTest::L2NodeTest() {

}

L2NodeTest::~L2NodeTest() {};

void L2NodeTest::SetUp() {};

void L2NodeTest::TearDown() {};

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


