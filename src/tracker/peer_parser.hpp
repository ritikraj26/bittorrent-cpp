#pragma once
#include <string>
#include <vector>

void print_peers(const std::string& peers_blob);

std::vector<std::string> parse_peers(const std::string& peers_blob);