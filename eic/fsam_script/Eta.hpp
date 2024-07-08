#ifndef ETA_HPP
#define ETA_HPP

#include <utility>
#include <vector>

#include "RangeParser.hpp"

class	Eta
{
public:
	Eta()
  {
    RangeParser rangeParser("eta_range", m_etaRange);

    size = m_etaRange.size() - 1;

    for (size_t i = 0; i < size; ++i) {
      m_middleValues[i] = (m_etaRange[i] + m_etaRange[i + 1]) / 2;
    }
  };
	~Eta() = default;
	Eta(const Eta& eta) = delete;
	Eta	&operator=(const Eta& eta) = delete;

  std::pair<double, double> operator[](int index)
  {
    return std::make_pair(getLowerBound(index), getUpperBound(index));
  };

  double getLowerBound(int index)
  {
    return m_etaRange.at(index);
  };
  double getUpperBound(int index)
  {
    return m_etaRange.at(index + 1);
  };
  double getMiddleValue(int index)
  {
    return m_middleValues[index];
  };

private:
  std::vector<double> m_etaRange;
  std::vector<double> m_middleValues;

public:
  size_t size;
};

#endif // ETA_HPP
