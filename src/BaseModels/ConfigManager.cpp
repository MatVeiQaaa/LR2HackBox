#include "BaseModels/ConfigManager.hpp"

#include <fstream>
#include <sstream>

ConfigManager::ConfigManager(std::string path, bool load) {
	ConfigManager::path = path;
	
	if (load) LoadConfig();
}

void ConfigManager::WriteValue(std::string name, std::string value) {
	config[name] = value;
}

std::string ConfigManager::ReadValue(std::string name) {
	return config[name];
}

bool ConfigManager::ValueExists(std::string name) {
	return config.contains(name);
}

void ConfigManager::SaveConfig() {
	std::ofstream file(path);
	for (auto& [name, value] : config) {
		file << name << " = " << value << std::endl;
	}
}

void ConfigManager::LoadConfig() {
	std::ifstream file(path);
	std::string line;
	while (std::getline(file, line))
	{
		std::string name = line.substr(0, line.find_first_of('='));
		name.erase(std::remove_if(name.begin(), name.end(), isspace), name.end());

		std::string value = line.substr(line.find_first_of('=') + 1, line.size() - 1);
		if (*value.begin() == ' ') value.erase(value.begin());

		config[name] = value;
	}
}