#ifndef HIST_HPP
#define HIST_HPP

#include "ROOT/RDataFrame.hxx"
#include "TGraph2DErrors.h"
#include "TH1D.h"
#include "TF1.h"

class	Hist
{
public:
// constructors & destructor
	Hist(const std::string& columnName, const ROOT::RDF::TH1DModel& hist1DInfo)
  : m_columnName(columnName), m_hist1DInfo(hist1DInfo)
  {
    m_graph2D = new TGraph2DErrors;
  }
	~Hist()
  {
    delete m_graph2D;
  }
	Hist(const Hist& hist);

// operators
	Hist	&operator=(const Hist& hist);

// member functions
  void drawCanvas()
  {
    if (m_hist1D.IsReady()) {
      std::string canvas_name = fmt::format("{} {}", sim_info, m_columnName);
      TCanvas *canvas = new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 700, 500);
      canvas->SetLogy(1);
      auto hist = histRecEnergy->DrawCopy();
      hist->SetLineWidth(2);
      hist->SetLineColor(kBlue);
      // TODO:
      save_canvas();
    }
  }
  void writeHist1D()
  {
    m_hist1D->Write();
  }
  void writeGraph2D()
  {
    m_graph2D->Write();
  }
  void setHist1D(const ROOT::RDF::RNode& dataNode) {
    m_hist1D = dataNode.Histo1D(
      m_hist1DInfo,
      m_columnName
    );
  }
  void setPoint(int i, double x, double y, double z)
  {
    m_graph2D->SetPoint(i, x, y, z);
  }
  void setPointError(int i, double ex, double ey, double ez)
  {
    m_graph2D->SetPointError(i, ex, ey, ez);
  }
  std::pair<double, double> getGausFitMean()
  {
    double upFit = m_hist1D->GetMean() + 5 * m_hist1D->GetStdDev();
    double downFit = m_hist1D->GetMean() - 5 * m_hist1D->GetStdDev();
    m_hist1D->Fit("gause", "", "", downFit, upFit);
    m_hist1D->GexXaxis()->SetRangeUser(downFit, upFit);
    TF1* gaus = hist->GetFunction("gaus");
    m_gausFitMean.first = gaus->GetParameter(1);
    m_gausFitMean.second = gaus->GetParError(1);
  }

  const std::string m_columnName;
  const ROOT::RDF::TH1DModel m_hist1DInfo;
  ROOT::RDF::RResultPtr<::TH1D> m_hist1D; // 
  TGraph2DErrors* m_graph2D;
  std::pair<double, double> m_gausFitMean; // { mean, error }
};

#endif // HIST_HPP
