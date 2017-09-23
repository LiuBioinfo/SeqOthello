#include "L2Node.hpp"


inline uint8_t get4b(uint8_t **pp, bool &hasvalue, uint8_t &buff) {
   if (hasvalue) {
       hasvalue = false;
       (*pp)++;
       return buff;
   }
   else {
      buff = ((**pp)>>4);
      hasvalue = true;
      return (**pp) & 0xF;
   }
}
inline uint8_t put4b(uint8_t **pp, bool &filledhalf, uint8_t val) {
   if (filledhalf) {
       (**pp) |= ((val & 0xF)<<4);
       (*pp)++;
       filledhalf = false;
   } else {
       (**pp) = (val & 0xF);
       filledhalf = true;
   }
}

uint32_t valuelistDecode(uint8_t *p, vector<uint32_t> &val, uint32_t maxmem) {
     uint8_t **pp = &p;
     val.clear();
     bool hasvalue = false; uint8_t buff = 0;
     while ( *pp < (p+maxmem) ) {
         uint8_t v = get4b(pp, hasvalue, buff);
         if (v & 0x8) {
             if (v == 8) return val.size();
             val.push_back(v&0x7);
             continue;
         }
         uint32_t v2 = get4b(pp, hasvalue, buff);
         if (v & 0x4) {
             uint32_t a = (v & 0x3);
             val.push_back( (a<<4) | v2);
             continue;
         }
         uint32_t v3 = get4b(pp, hasvalue, buff);
         if (v & 0x2) {
             uint32_t a = (v & 0x1);
             val.push_back( (a<<8) | (v2<<4) | v3);
             continue;
         }
         uint32_t v4 = get4b(pp, hasvalue, buff);
         if (v == 1) {
             val.push_back( (v2<<8) | (v3<<4) | v4);
             continue;
         }
         uint32_t v5 = get4b(pp, hasvalue, buff);
         uint32_t v6 = get4b(pp, hasvalue, buff);
         val.push_back( (v2<<16) | (v3<<12) | (v4<<8) | (v5<<4) | v6);
     }
     return val.size();
}

uint32_t valuelistEncode(uint8_t *p, vector<uint32_t> &val, bool really) {
    uint32_t ans = 0;
    
    uint8_t **pp = &p;
    uint8_t *p0 = p;
    bool filledhalf = false;
    //inline uint8_t put4b(uint8_t **pp, bool &filledhalf, uint8_t val) {
    for (auto &x: val) {
        if (x>0xFFF) { //>12bits
           if (really) {
           put4b(pp, filledhalf, 0);
           put4b(pp, filledhalf, 0xF & (x >> 16));
           put4b(pp, filledhalf, 0xF & (x >> 12));
           put4b(pp, filledhalf, 0xF & (x >> 8));
           put4b(pp, filledhalf, 0xF & (x >> 4));
           put4b(pp, filledhalf, 0xF & x);
           }
           else ans += 6;
        }
        else if (x>0x1FF) { //10~12bits
           if (really) {
           put4b(pp, filledhalf, 1);
           put4b(pp, filledhalf, 0xF & (x >> 8));
           put4b(pp, filledhalf, 0xF & (x >> 4));
           put4b(pp, filledhalf, 0xF & x);
           }
           else ans += 4;
        }
        else if (x>0x3F) { // 7~9 bits
           if (really) {
           put4b(pp, filledhalf, 2 | (x>>8) );
           put4b(pp, filledhalf, 0xF & (x >> 4));
           put4b(pp, filledhalf, 0xF & x);
           }
           else ans += 3;
        }
        else if (x>0x7) { // 4~6 bits
           if (really) {
           put4b(pp, filledhalf, 4 | (x>>4) );
           put4b(pp, filledhalf, 0xF & x);
           } 
           else ans += 2;
        }
        else { //<=3bits
           if (really)
               put4b(pp, filledhalf, 0x8 | x);
           else 
               ans ++;
        }
    }
    if (really) {
        put4b(pp, filledhalf, 8);
        return (p-p0)*2 + (filledhalf);
    }
    ans ++;
    return ans;
}
