#ifndef ENERGY_HPP
#define ENERGY_HPP

#include <vector>
#include <string>

#include "RangeParser.hpp"

class Energy
{
public:
	Energy(const std::string& particle)
  : m_particleName(particle)
  {
    RangeParser rangeParser(particle + "_range", m_energyBins);
  };
	~Energy() = default;
	Energy(const Energy& energy) = delete;
	Energy& operator=(const Energy& energy) = delete;

  const std::vector<double>& getEnergyBins() const 
  {
    return m_energyBins;
  };
private:
  std::string         m_particleName;
  std::vector<double> m_energyBins;
};

#endif // ENERGY_HPP
