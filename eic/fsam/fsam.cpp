#include <string>
#include <iostream>

#include "HistManager.hpp"

int fsam(std::string pathPrefix)
{
  if (pathPrefix.back() != '/') {
    pathPrefix.push_back('/');
  }
  std::cout << pathPrefix << '\n';
  HistManager histManager{pathPrefix};

  histManager.process();
  histManager.storeHists();
  return 0;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
	return 1;
    }
    fsam(argv[1]);
    return 0;
}
