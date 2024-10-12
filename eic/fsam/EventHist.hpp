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
  EventHist(
    const std::string& columnName,
    const ROOT::RDF::TH1DModel& columnInfo,
    const std::string& simInfo)
    : m_columnName(columnName)
    , m_simInfo(simInfo)
    , m_columnInfo(columnInfo)
  {
    std::string name = fmt::format("{}_{}", m_columnName, simInfo);
    m_columnInfo.fName = name;
  }
  ~EventHist() {}
  EventHist(const EventHist& hist) = delete;

  // operators
  EventHist& operator=(const EventHist& hist) = delete;

  // member functions
  std::pair<double, double> getGausFitMean(ROOT::RDF::RNode& dataNode, bool draw = false)
  {
    TCanvas* cvs;
    static std::mutex mtx;

    (void)cvs;
    m_hist1D = dataNode.Histo1D(m_columnInfo, m_columnName);
    auto nEvents = dataNode.Count();
    (void)*nEvents;
    // std::cout << ".";
    while (m_hist1D.IsReady() == kFALSE)
      std::cout << "hist1D is not ready\r";
    std::cout << '\n';
    mtx.lock();
    if (draw) {
      cvs = new TCanvas(m_columnInfo.fName, m_columnInfo.fName, 700, 500);
    }
    auto hist1D = m_hist1D->DrawCopy();
    hist1D->SetLineWidth(2);
    hist1D->SetLineColor(kBlue);
    double upFit = hist1D->GetMean() + 5 * hist1D->GetStdDev();
    double downFit = hist1D->GetMean() - 5 * hist1D->GetStdDev();
    hist1D->Fit("gaus", "", "", downFit, upFit);
    hist1D->GetXaxis()->SetRangeUser(downFit, upFit);
    TF1* gaus = hist1D->GetFunction("gaus");
    std::pair<double, double> gausFitMean;
    gausFitMean.first = gaus->GetParameter(1);
    gausFitMean.second = gaus->GetParError(1);
    gaus->SetLineWidth(2);
    gaus->SetLineColor(kRed);
    if (draw) {
      cvs->SaveAs(fmt::format("{}{}.pdf", s_pathPrefix, m_simInfo).c_str());
    }
    mtx.unlock();
    return gausFitMean;
  }

private:
  const std::string m_columnName;
  const std::string m_simInfo;
  ROOT::RDF::TH1DModel m_columnInfo;
  ROOT::RDF::RResultPtr<::TH1D> m_hist1D;

public:
  static std::string s_pathPrefix;
};

#endif // EVENTHIST_HPP
