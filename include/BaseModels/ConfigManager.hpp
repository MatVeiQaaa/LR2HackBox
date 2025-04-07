#pragma once

#include <windows.h>
#include <string>
#include <map>

class ConfigManager {
public:
	ConfigManager(std::string path, bool load = true);
	void WriteValue(std::string name, std::string value);
	std::string ReadValue(std::string name);

	void SaveConfig();
	void LoadConfig();
private:
	std::string path;
	std::map<std::string, std::string> config;
};