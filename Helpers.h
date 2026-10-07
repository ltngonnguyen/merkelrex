// ADDITION #15 - HELPER FUNCTIONS HEADER FILE

#pragma once

#include "CSVReader.h"
#include <iomanip>
#include <string>
#include <vector>

// Function prototypes
std::vector<std::string> processTimestamp(std::string raw_timestamp);
std::string typewriter(int index, std::string text);
long double mapping(long double val, long double i_min, long double i_max,
                    long double o_min, long double o_max);
std::vector<long double> processMappingUpper(long double value);
std::vector<long double> processMappingLower(long double value);
std::string processRemainder(long double remainder, std::string position);
std::string shortenNumber(long long number);
std::string colorize(std::string text, std::string color);
