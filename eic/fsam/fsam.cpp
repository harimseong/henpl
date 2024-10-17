#include <iostream>
#include <string>

#include <fmt/core.h>

#include "HistManager.hpp"

void
getFsam(std::string recPath, std::string edepPath);

int
fsam(std::string pathPrefix)
{
  bool isSensitive = false;

  if (pathPrefix.back() != '/') {
    pathPrefix.push_back('/');
  }
  std::cout << pathPrefix << '\n';
  if (pathPrefix.find("sensitive") == std::string::npos) {
    std::cout << "reconstructed hits will be computed\n";
  } else {
    std::cout << "energy deposit will be computed\n";
    isSensitive = true;
  }

  HistManager histManager{ pathPrefix, isSensitive };

  histManager.process();
  histManager.storeHists();
  return 0;
}

int
main(int argc, char** argv)
{
  if (argc == 2) {
    std::cout << "Generating ROOT\n";
    fsam(argv[1]);
  } else if (argc == 3) {
    std::cout << "Computing samping fraction\n";
    getFsam(argv[1], argv[2]);
  } else {
    std::cerr << fmt::format("usage: {} PATH1 [PATH2]\n", argv[0]);
    std::cerr << fmt::format(
"\n\
1) if PATH2 is not given, it will generate ROOT file using data in the PATH1.\n\
   if PATH1 contains \"sensitive\" keyword, it will compute deposit energy.\n\
");
    std::cerr << fmt::format(
"\n\
2) if PATH1 and PATH2 are given, it will generate sampling fraction ROOT file\n\
   using PATH1 as reconstructed energy sum and PATH2 as deposit energy sum\n\
");
    return 1;
    std::cerr << "invalid arguments\n";
  }
  return 0;
}
