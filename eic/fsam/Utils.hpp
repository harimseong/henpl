#ifndef RANGEPARSER_HPP
#define RANGEPARSER_HPP

#include <string>
#include <tuple>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

static std::vector<double>
parseRange(std::string filename)
{
  std::ifstream ifs(filename);
  std::vector<double> range;

  while (true) {
    double val;
    ifs >> val;
    if (ifs.fail() || ifs.eof())
      break;
    range.push_back(val);
  }
  if (!ifs.eof()) {
    std::cerr << "Failed to read range file.\n";
    range.clear();
  }
  if (0) {
    std::cout << "Range from " << filename << ":\n";
    int i = 1;
    std::for_each(range.begin(), range.end(), 
    [&i](double num) {
      std::cout << i++ << ": " << num << '\n';
    });
    std::cout << '\n';
  }
  return range;
}

/*
static std::tuple<std::string, unsigned int, unsigned int>
parsePathprefix(std::string pathprefix)
{
  std::tuple<std::string, unsigned int, unsigned int> tup;
  std::stringstream ifs(pathprefix);
  char temp[32];

  ifs.getline(temp, 32, '_');
  std::get<0>(tup) = temp;
  ifs.getline(temp, 32, '_');
  std::get<1>(tup) = std::stoi(temp);
  ifs.getline(temp, 32);
  std::get<2>(tup) = std::stoi(temp);
  if (ifs.fail()) {
    std::cerr << "Failed to parse filename.\n";
  } else {
    std::cout << "Particle: " << std::get<0>(tup) << '\n'
              << "Date: " << std::get<1>(tup) << '\n'
              << "Time: " << std::get<2>(tup) << '\n';
  }
  return tup;
}
*/

#endif // RANGEPARSER_HPP
