#ifndef EVENTHIST_HPP
#define EVENTHIST_HPP

#include <fmt/core.h>
#include <mutex>

#include "ROOT/RDataFrame.hxx"
#include "TCanvas.h"
#include "TF1.h"
#include "TGraph2DErrors.h"
#include "TH1D.h"

class EventHist
{
public:
  // constructors & destructor
  EventHist(const std::string& columnName,
            const ROOT::RDF::TH1DModel& columnInfo,
            const std::string& simInfo);
  ~EventHist() {}
  EventHist(const EventHist& hist) = delete;

  // operators
  EventHist& operator=(const EventHist& hist) = delete;

  // member functions
  std::pair<double, double>
  getGausFitMean(ROOT::RDF::RNode& dataNode, bool draw);

private:
  const std::string m_columnName;
  const std::string m_simInfo;
  ROOT::RDF::TH1DModel m_columnInfo;
  ROOT::RDF::RResultPtr<TH1D> m_hist1D;

public:
  static std::string s_pathPrefix;
};

#endif // EVENTHIST_HPP
