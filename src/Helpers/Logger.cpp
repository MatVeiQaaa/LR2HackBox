#include "Logger.hpp"

#include <string>
#include <fstream>
#include <iomanip>

void Logger::LogOut(std::string buffer) {
    std::ofstream logFile;
    logFile.open(Logger::mLogPath, std::ios_base::app);
    std::time_t timeStamp = std::time(NULL);
    logFile << std::put_time(std::localtime(&timeStamp), "%D %T ") << buffer;
    logFile << std::endl << std::endl;
    logFile.close();
}

void Logger::SetPath(std::string path) {
    mLogPath = path;
}