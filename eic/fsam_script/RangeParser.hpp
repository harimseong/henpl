#ifndef RANGEPARSER_HPP
#define RANGEPARSER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

class	RangeParser
{
public:
	RangeParser(std::string fileName, std::vector<double>& range)
  {
    std::ifstream ifs(fileName);

    while (!ifs.fail() && !ifs.eof()) {
      double energy;
      ifs >> energy;
      range.push_back(energy);
    }
    if (!ifs.eof()) {
      std::cerr << "Failed to read range file.\n";
      range.clear();
    }
  };
	~RangeParser();
	RangeParser(const RangeParser& rangeparser) = delete;
	RangeParser	&operator=(const RangeParser& rangeparser) = delete;
};

#endif // RANGEPARSER_HPP
