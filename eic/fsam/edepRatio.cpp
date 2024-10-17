#include <iostream>
#include <vector>

#include "TFile.h"
#include "TH1.h"
#include "TH1D.h"
#include "TGraph2DErrors.h"
#include "ROOT/RDataFrame.hxx"

#include "fmt/core.h"
#include "Energy.hpp"
#include "Eta.hpp"

void
edepRatio(std::string edepPath);

int
main(int argc, char** argv)
{
  if (argc != 2) {
    std::cerr << "invalid arguments\n";
    return 1;
  }
  edepRatio(argv[1]);
  return 0;
}

void
edepRatio(std::string edepPath)
{
  if (edepPath.back() != '/') {
    edepPath.push_back('/');
  }

  Eta etaBins{edepPath + "ETA_range"};
  Energy energyBins{edepPath + "E_range"};

  std::string resultPath = edepPath;
  std::string ratioPath = resultPath + "edepRatio1DHists.root";
  edepPath += "1DHists.root";
  std::vector<TH1D*> ratioEHists;
  std::vector<TH1D*> ratioEtaHists;
  TFile* edepFile = TFile::Open(edepPath.c_str(), "READ");
  TFile* ratioFile = TFile::Open(ratioPath.c_str(), "CREATE");
  TGraph2DErrors* graph = new TGraph2DErrors();

  std::cout << "energy bins\n";
  for (size_t i = 0; i < energyBins.size(); ++i) {
    std::cout << energyBins[i] << '\n';
  }
  std::cout << "eta bins\n";
  for (size_t i = 0; i < etaBins.size(); ++i) {
    std::cout << etaBins.getMiddleValue(i) << '\n';
  }

  if (edepFile == nullptr || edepFile->IsOpen() == kFALSE) {
    std::cerr << fmt::format("cannot open file {}\n", edepPath);
    return;
  }
  if (ratioFile == nullptr || ratioFile->IsOpen() == kFALSE) {
    std::cerr << fmt::format("cannot open file {}\n", ratioPath);
    return;
  }

  for (size_t i = 0; i < energyBins.size(); ++i) {
    std::string histName{fmt::format("E{}", energyBins[i])};
    std::cout << "reading " << histName << '\n';

    TH1D* edepHist = edepFile->Get<TH1D>(histName.c_str());
    TH1D* ratioHist = new TH1D(
      histName.c_str(),
      histName.c_str(),
      etaBins.size(),
      etaBins.getBinEdges());
    ratioEHists.push_back(ratioHist);
    if (ratioHist == nullptr) {
      std::cerr << "failed to allocate TH1D\n";
      return;
    }
    for (size_t j = 0; j < etaBins.size(); ++j) {
      double edep = edepHist->GetBinContent(j + 1);
      double edepError = edepHist->GetBinError(j + 1);
      double ratio;
      double ratioError;

      if (edep < 0.000001) {
        ratio = 0.;
        ratioError = 0.;
      } else {
        ratio = edep / energyBins[i];
        ratioError = edepError / energyBins[i];
      }
      ratioHist->SetBinContent(j + 1, ratio);
      ratioHist->SetBinError(j + 1, ratioError);
      graph->SetPoint(
        i * etaBins.size() + j,
        etaBins.getMiddleValue(j),
        energyBins[i], ratio);
      graph->SetPointError(
        i * etaBins.size() + j,
        etaBins.getMiddleValue(j),
        energyBins[i], ratioError);
    }
    delete edepHist;
  }

  for (size_t i = 0; i < etaBins.size(); ++i) {
    std::string histName{fmt::format("eta{}", etaBins.getMiddleValue(i))};
    std::cout << "reading " << histName << '\n';

    TH1D* edepHist = edepFile->Get<TH1D>(histName.c_str());
    TH1D* ratioHist = new TH1D(
      histName.c_str(),
      histName.c_str(),
      energyBins.size(),
      energyBins.getBinEdges());
    ratioEtaHists.push_back(ratioHist);
    if (ratioHist == nullptr) {
      std::cerr << "failed to allocate TH1D\n";
      return;
    }
    std::cout << "\neta=" << etaBins.getMiddleValue(i) << '\n';
    for (size_t j = 0; j < energyBins.size(); ++j) {
      double edep = edepHist->GetBinContent(j + 1);
      double edepError = edepHist->GetBinError(j + 1);
      double ratio;
      double ratioError;

      if (edep == 0.) {
        ratio = 0.;
        ratioError = 0.;
      } else {
        ratio = edep / energyBins[j];
        ratioError = edepError / energyBins[j];
        std::cout << fmt::format("edep={:.4}\tebin={:5}\tratio={}\n", edep, energyBins[j], ratio);
      }
      ratioHist->SetBinContent(j + 1, ratio);
      ratioHist->SetBinError(j + 1, ratioError);
    }
    delete edepHist;
  }
  graph->SetTitle("Deposit Energy to True Energy Ratio; Eta; Energy; Ratio");
  graph->SaveAs(fmt::format("{}edepRatio2Dgraph.root", resultPath).c_str());
  ratioFile->cd();
  for (TH1D* hist : ratioEHists) {
    hist->Write();
  }
  for (TH1D* hist : ratioEtaHists) {
    hist->Write();
  }
  ratioFile->Close();
}
