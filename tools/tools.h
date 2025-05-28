#ifndef TOOLS_H
#define TOOLS_H
#include <string>
#include <iostream>
#include <iomanip>

std::string getFilenameFromPath(const std::string& filepath);
void printProgressBar(int percent, int barWidth = 50);
#endif //TOOLS_H