#include "util.h"
#include <cstring>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <iostream>


//! split a c-style string with delimineter chara.
std::vector<std::string> split(const char * str, char deli) {
    std::istringstream ss(str);
    std::string token;
    std::vector<std::string> ret;
    while(std::getline(ss, token, deli)) {
        if (token.size()>=1)
            ret.push_back(token);
    }
    return ret;
}
void printcurrtime() {
    auto end = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    printf("%s ::", std::ctime(&end_time));
}


//! convert a 64-bit Integer to human-readable format in K/M/G. e.g, 102400 is converted to "100K".
std::string human(uint64_t word) {
    std::stringstream ss;
    if (word <= 1024) ss << word;
    else if (word <= 10240) ss << std::setprecision(2) << word*1.0/1024<<"K";
    else if (word <= 1048576) ss << word/1024<<"K";
    else if (word <= 10485760) ss << word*1.0/1048576<<"M";
    else if (word <= (1048576<<10)) ss << word/1048576<<"M";
    else ss << word*1.0/(1<<30) <<"G";
    std::string s;
    ss >>s;
    return s;
}

std::string get_thid() {
    std::stringstream ss;
    ss <<"Thread_" <<std::hex<< std::this_thread::get_id();
    std::string s;
    ss >> s;
    return s;
}
