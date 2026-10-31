#include "script.h"
#include <string>
#include <vector>
#include <fstream>

// 加载剧本
std::vector<dialogue> loadScript(const std::string& path)
{
    // 检验，避免空跑
    std::ifstream script(path);
    if (!script)
        return {};

    // 单段发言
    std::vector<dialogue> lines;
    std::string rawBuf;
    std::string speakerBuf;
    std::string textBuf;
    while (std::getline(script, rawBuf)) {
        if (rawBuf.empty())
            continue;

        if (rawBuf.starts_with('#') && rawBuf.ends_with('#')) {
            speakerBuf = rawBuf.substr(1, rawBuf.size() - 2);
            continue;
        }
        else
            textBuf = rawBuf;

        lines.push_back({ speakerBuf, textBuf });
    }

    return lines;
}