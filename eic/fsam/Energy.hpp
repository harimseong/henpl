#ifndef ENERGY_HPP
#define ENERGY_HPP

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

    if (m_size < 2) {
      return;
    }
    // first bin low-edge
    m_binEdges.push_back(m_bins[0] - (m_bins[1] - m_bins[0]) / 2);
    for (size_t i = 1; i < m_size; ++i) {
      m_binEdges.push_back(m_bins[i] - (m_bins[i] - m_bins[i - 1]) / 2);
    }
    // last bin up-edge
    m_binEdges.push_back(m_bins.back() + (m_bins.back() - m_bins[m_size - 2]) / 2);
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

  const double* getBinEdges() const
  {
    return m_binEdges.data();
  };

  void printBins() const
  {
    std::cout << "energy bins:\n";
    for (size_t i = 0; i < m_bins.size(); ++i) {
      std::cout << m_bins[i] << '\n';
    }
    std::cout << '\n';
  };

  void printBinEdges() const
  {
    std::cout << "energy bin edges:\n";
    for (size_t i = 0; i < m_binEdges.size(); ++i) {
      std::cout << m_binEdges[i] << '\n';
    }
    std::cout << '\n';
  };

private:
  std::vector<double> m_bins;
  std::vector<double> m_binEdges;

public:
  size_t m_size;
};

#endif // ENERGY_HPP
