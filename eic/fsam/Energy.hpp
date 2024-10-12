#ifndef ENERGY_HPP
#define ENERGY_HPP

#include <cctype>
#include <vector>
#include <string>

#include "Utils.hpp"

class Energy
{
public:
	Energy(const std::string& fileName)
  {
    m_bins = parseRange(fileName);
    m_size = m_bins.size();
  };
	~Energy() = default;
	Energy(const Energy& energy) = delete;
	Energy& operator=(const Energy& energy) = delete;

  double operator[](int i) const
  {
    return m_bins.at(i);
  }

  const std::vector<double>& getEnergyBins() const 
  {
    return m_bins;
  };

  size_t size() const
  {
    return m_size;
  };

private:
  std::vector<double> m_bins;

public:
  size_t m_size;
};

#endif // ENERGY_HPP
