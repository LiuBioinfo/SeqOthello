#include <io_helper.hpp>
using namespace std;
int main(int argc, char ** argv) {
        auto readerA = new BinaryKmerReader<uint64_t>(argv[1]);
        auto readerB = new BinaryKmerReader<uint64_t>(argv[2]);
        uint64_t va=0, vb=0;
        bool ra = true, rb=true;
        int CAP = 0, DA =0, DB = 0;
        while (ra && rb) {
                if (va == vb) {
                        uint64_t v0 = va;
                        uint32_t cnta=0,cntb=0;
                        while (va == v0 && ra) {
                            ra = readerA->getNext(&va);
                            cnta++;
                        }
                        while (vb == v0 && rb) {
                            rb = readerB->getNext(&vb);
                            cntb++;
                        }
                        CAP+= (cnta <cntb)?cntb:cnta;
                        continue;
                }
                if (va < vb) {
                        DA++;
                        ra = readerA->getNext(&va);
                }
                else {
                        DB++;
                        rb = readerB->getNext(&vb);
                }
        }
        while (ra) { DA++; ra = readerA->getNext(&va);}
        while (rb) { DB++; rb = readerB->getNext(&vb);}
        printf("%d\t%d\t%d\n", CAP,DA,DB);
        return 0;
}
