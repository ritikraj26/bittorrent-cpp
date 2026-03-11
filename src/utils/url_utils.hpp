#pragma once

#include <string>
#include <cctype>
#include <sstream>
#include <iomanip>


std::string url_decode(const std::string& input);

std::string url_encode(const std::string& data);