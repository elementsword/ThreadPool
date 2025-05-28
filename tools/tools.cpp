#include "tools.h"

std::string getFilenameFromPath(const std::string &filepath)
{
    // 查找最后一个斜杠或反斜杠的位置
    size_t pos = filepath.find_last_of("/\\");
    if (pos == std::string::npos)
        return filepath; // 没有斜杠，说明就是文件名
    return filepath.substr(pos + 1);
}

void printProgressBar(int percent, int barWidth)
{
    std::cout << "\r上传中：[";
    int pos = (percent * barWidth) / 100;
    for (int i = 0; i < barWidth; ++i)
    {
        if (i < pos)
            std::cout << "#";
        else
            std::cout << "-";
    }
    std::cout << "] " << std::setw(3) << percent << "%" << std::flush;
}