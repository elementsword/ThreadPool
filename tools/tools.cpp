#include "tools.h"

std::string getFilenameFromPath(const std::string& filepath) {
    // 查找最后一个斜杠或反斜杠的位置
    size_t pos = filepath.find_last_of("/\\");
    if (pos == std::string::npos)
        return filepath; // 没有斜杠，说明就是文件名
    return filepath.substr(pos + 1);
}