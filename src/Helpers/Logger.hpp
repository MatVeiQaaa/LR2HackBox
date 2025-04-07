#include <string>

class Logger {
public:
	void LogOut(std::string buffer);
	void SetPath(std::string path);
private:
	std::string mLogPath = "default.log";
};
