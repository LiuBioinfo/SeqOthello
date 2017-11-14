// This file is a part of SeqOthello. Please refer to LICENSE.TXT for the LICENSE
#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <thread>

std::string human(uint64_t word);

std::vector<std::string> split(const char * str, char deli);
void printcurrtime();

std::string get_thid();
