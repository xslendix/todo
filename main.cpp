#include <iostream>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/config.hpp>
#include <boost/program_options/environment_iterator.hpp>
#include <boost/program_options/eof_iterator.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/version.hpp>

namespace po = boost::program_options;

// HELPERS
bool ynPrompt(std::string prompt, char def='y');
std::filesystem::path getConfPath();
void delete_line(std::filesystem::path file_name, int n);
static bool is_match(const std::string& text, const std::string& pattern);

// OPTIONS
void initDB(std::filesystem::path dbPath);
void listDB(std::filesystem::path dbPath);
void addDB(std::filesystem::path dbPath, std::string text);
void removeDB(std::filesystem::path dbPath, int index);
void searchDB(std::filesystem::path dbPath, std::string pattern);
void cleanDB(std::filesystem::path dbPath);

int main(int argc, const char** argv) {

	std::string text;
	int index;

	po::options_description desc("Available Options");
	po::options_description ddesc("Dangerous Options");

	ddesc.add_options()
		("clean,c", "Wipes the entire database. Might need re-initialization.")
	;
	desc.add(ddesc).add_options()
		("help,h", "Displays the help message.")
		("init,i", "Initializes the database.")
		("list,l", "Lists all the items in the database.")
		("add,a", po::value<std::string>(&text), "Add item to the database.")
		("remove,r", po::value<int>(&index), "Remove item from the database from the given index.")
		("search,s", po::value<std::string>(&text), "Searches the database for the given text")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);    

	if (argc == 1 || vm.count("help")) {
		std::cout << desc << "\n";
		return 0;
	}

	if (vm.count("init"))
		initDB(getConfPath() / "database");
	else if (vm.count("list"))
		listDB(getConfPath() / "database");
	else if (vm.count("add"))
		addDB(getConfPath() / "database", text);
	else if (vm.count("remove"))
		removeDB(getConfPath() / "database", index);
	else if (vm.count("search"))
		searchDB(getConfPath() / "database", text);
	// DANGEROUS
	else if (vm.count("clean"))
		cleanDB(getConfPath() / "database");

	return 0;

}

// OPTIONS
void initDB(std::filesystem::path dbPath) {
	bool lock = false;
	bool exists = false;
	if ((exists = std::filesystem::exists(dbPath)))
		lock = !ynPrompt("Database already exists, re-initializing the database will delete everything. Do you want to continue?");
	if (lock) return;

	std::ofstream f;
	f.open(dbPath);
	f<<"";
	f.close();

	std::cout << "Database " << (exists ? "re-" : "") << "initialized!\n";
}

void listDB(std::filesystem::path dbPath) {
	if (!std::filesystem::exists(dbPath)) {
		std::cout << "Database not initialized!\n";
		exit(1);
	}

	std::ifstream f;
	f.open(dbPath);
	
	std::string line;
	int i = 1;
	while(std::getline(f, line)) {
		if(!line.empty()) {
			std::cout << i << ". " << line << '\n';
			i++;
		}
	}
}

void addDB(std::filesystem::path dbPath, std::string text) {
	if (!std::filesystem::exists(dbPath)) {
		std::cout << "Database not initialized!\n";
		exit(1);
	}

	std::fstream f(dbPath, std::ios::out | std::ios::app);
	f << '\n' << text;
	f.close();
}

void removeDB(std::filesystem::path dbPath, int index) {
	if (!std::filesystem::exists(dbPath)) {
		std::cout << "Database not initialized!\n";
		exit(1);
	}

	delete_line(dbPath, index+1);
}

void searchDB(std::filesystem::path dbPath, std::string pattern) {
	if (!std::filesystem::exists(dbPath)) {
		std::cout << "Database not initialized!\n";
		exit(1);
	}

	std::ifstream f;
	f.open(dbPath);
	
	std::string line;
	int i = 1;
	while(std::getline(f, line)) {
		if(!line.empty()) {
			if (is_match(line, pattern))
				std::cout << i << ". " << line << '\n';
			i++;
		}
	}
}

void cleanDB(std::filesystem::path dbPath) {
	if (!std::filesystem::exists(dbPath)) {
		std::cout << "Database not initialized!\n";
		exit(1);
	}

	std::filesystem::remove(dbPath);
	std::cout << "Database cleaned!\n";
}

// HELPERS
bool ynPrompt(std::string prompt, char def) {
	std::cout << prompt << (def=='y' ? " [Y/n]: " : " [y/N]: ");
	char inp;
	inp = std::getchar();
	if (inp == '\n') inp = def;
	
	return std::tolower(inp) == 'y';
}

std::filesystem::path getConfPath() {
	std::filesystem::path home = std::getenv("HOME");
	std::filesystem::path config_home;

	char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
	if (std::string(xdg_config_home).empty())
		config_home = home / ".todo";
	else
		config_home = std::filesystem::path(xdg_config_home) / "todo";

	if (!std::filesystem::exists(config_home))
		std::filesystem::create_directory(config_home);

	return config_home;
}

void delete_line(std::filesystem::path file_name, int n)  { 
	// open file in read mode or in mode 
	std::ifstream is(file_name); 

	// open file in write mode or out mode 
	std::ofstream ofs; 
	ofs.open("/tmp/tmptodo.txt", std::ofstream::out); 

	// loop getting single characters 
	char c; 
	int line_no = 1; 
	while (is.get(c)) { 
		// if a newline character 
		if (c == '\n') 
			line_no++; 

		// file content not to be deleted 
		if (line_no != n) 
			ofs << c; 
	} 

	// closing output file 
	ofs.close(); 

	// closing input file 
	is.close(); 

	// remove the original file 
	std::filesystem::remove(file_name); 

	// rename the file 
	std::filesystem::copy(std::filesystem::path("/tmp/tmptodo.txt"), file_name); 

} 

static bool is_match(const std::string& text, const std::string& pattern) {
    return std::string::npos != text.find(pattern);
}
