#pragma once
#include <string>
#include <vector>

struct dialogue {
    std::string speaker = "";
    std::string text = "";
};

std::vector<dialogue> loadScript(const std::string& path);