#include "EventHist.hpp"

std::string EventHist::s_pathPrefix;

EventHist::EventHist(
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

// member functions
std::pair<double, double>
EventHist::getGausFitMean(ROOT::RDF::RNode& dataNode, bool draw = false)
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
  TH1D* hist1D = static_cast<TH1D*>(m_hist1D->Clone(m_columnInfo.fName));
  hist1D->SetLineWidth(2);
  hist1D->SetLineColor(kBlue);
  hist1D->Draw("PE");
  std::pair<double, double> gausFitMean;
  double upFit = hist1D->GetMean() + 5. * hist1D->GetStdDev();
  double downFit = hist1D->GetMean() - 1. * hist1D->GetStdDev();
  double up = hist1D->GetMean() + 5. * hist1D->GetStdDev();
  double down = hist1D->GetMean() - 5. * hist1D->GetStdDev();
  Int_t fitResult = hist1D->Fit("gaus", "L", "", downFit, upFit);
  std::cout << "fitResult=" << fitResult << '\n';
  if (fitResult < 0) {
    std::cout
      << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
    std::cout << "fitResult < 0\n";
    std::cout
      << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
    gausFitMean.first = 0;
    gausFitMean.second = 0;
  } else {
    hist1D->GetXaxis()->SetRangeUser(down, up);
    TF1* gaus = hist1D->GetFunction("gaus");
    gausFitMean.first = gaus->GetParameter(1);
    if (gausFitMean.first < 0) {
      std::cout << "mean value is less than 0. function will return 0.\n";
      gausFitMean.first = 0;
      gausFitMean.second = 0;
    }
    gausFitMean.second = gaus->GetParError(1);
    gaus->SetLineWidth(2);
    gaus->SetLineColor(kRed);
  }
  std::cout << "simInfo=" << m_simInfo << '\n';
  if (draw) {
    cvs->SaveAs(
      fmt::format("{}{}_{}.pdf", s_pathPrefix, m_simInfo, m_columnName).c_str());
  }
  mtx.unlock();
  return gausFitMean;
}
