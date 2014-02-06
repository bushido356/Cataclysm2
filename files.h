#ifndef _FILES_H_
#define _FILES_H_

#include <vector>
#include <string>
#include <istream>

extern std::string DATA_DIR;
extern std::string CUSS_DIR;
// TODO: extern std::string PROFILE_DIR;

bool directory_exists(std::string name);
bool file_exists     (std::string name);
std::vector<std::string> files_in(std::string dir, std::string suffix = "");
std::vector<std::string> directories_in(std::string dir);

std::string slurp_file(const std::string &filename);

void chomp(std::istream &data);

void set_default_dirs();
bool set_data_dir(std::string dir); // Returns false if the dir doesn't exist

#endif
