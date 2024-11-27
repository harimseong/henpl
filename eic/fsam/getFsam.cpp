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
getFsam(std::string recPath, std::string edepPath)
{
  if (recPath.back() != '/') {
    recPath.push_back('/');
  }
  if (edepPath.back() != '/') {
    edepPath.push_back('/');
  }

  Eta etaBins{recPath + "ETA_range"};
  Energy energyBins{recPath + "E_range"};

  std::string resultPath = edepPath;
  std::string fsamPath = resultPath + "fsam1DHists.root";
  recPath += "1DHists.root";
  edepPath += "1DHists.root";
  std::vector<TH1D*> fsamEHists;
  std::vector<TH1D*> fsamEtaHists;
  TFile* recFile = TFile::Open(recPath.c_str(), "READ");
  TFile* edepFile = TFile::Open(edepPath.c_str(), "READ");
  TFile* fsamFile = TFile::Open(fsamPath.c_str(), "CREATE");
  TGraph2DErrors* graph = new TGraph2DErrors();

  if (recFile == nullptr || recFile->IsOpen() == kFALSE) {
    std::cerr << fmt::format("cannot open file {}\n", recPath);
    return;
  }
  if (edepFile == nullptr || edepFile->IsOpen() == kFALSE) {
    std::cerr << fmt::format("cannot open file {}\n", edepPath);
    return;
  }
  if (fsamFile == nullptr || fsamFile->IsOpen() == kFALSE) {
    std::cerr << fmt::format("cannot open file {}\n", fsamPath);
    return;
  }

  for (size_t i = 0; i < energyBins.size(); ++i) {
    std::string histName{fmt::format("E{}", energyBins[i])};
    std::cout << "reading " << histName << '\n';

    TH1D* recHist = recFile->Get<TH1D>(histName.c_str());
    TH1D* edepHist = edepFile->Get<TH1D>(histName.c_str());
    TH1D* fsamHist = new TH1D(
      histName.c_str(),
      histName.c_str(),
      etaBins.size(),
      etaBins.getBinEdges());
    fsamEHists.push_back(fsamHist);
    if (recHist == nullptr || edepHist == nullptr) {
      std::cerr << "failed to read histogram\n";
      return;
    }
    if (fsamHist == nullptr) {
      std::cerr << "failed to allocate TH1D\n";
      return;
    }
    for (size_t j = 0; j < etaBins.size(); ++j) {
      double rec = recHist->GetBinContent(j + 1);
      double recError = recHist->GetBinError(j + 1);
      double edep = edepHist->GetBinContent(j + 1);
      // double edepError = edepHist->GetBinError(j);
      double fsam;
      double fsamError;

      if (edep < 0.000001) {
        fsam = 0.;
        fsamError = 0.;
      } else {
        fsam = rec / edep;
        fsamError = recError / edep;
      }
      fsamHist->SetBinContent(j + 1, fsam);
      fsamHist->SetBinError(j + 1, fsamError);
      graph->SetPoint(
        i * etaBins.size() + j,
        etaBins.getMiddleValue(j),
        energyBins[i], fsam);
      graph->SetPointError(
        i * etaBins.size() + j,
        etaBins.getMiddleValue(j),
        energyBins[i], fsamError);
    }
    delete recHist;
    delete edepHist;
  }

  for (size_t i = 0; i < etaBins.size(); ++i) {
    std::string histName{fmt::format("eta{}", etaBins.getMiddleValue(i))};
    std::cout << "reading " << histName << '\n';

    TH1D* recHist = recFile->Get<TH1D>(histName.c_str());
    TH1D* edepHist = edepFile->Get<TH1D>(histName.c_str());
    TH1D* fsamHist = new TH1D(
      histName.c_str(),
      histName.c_str(),
      energyBins.size(),
      energyBins.getBinEdges());
    fsamEtaHists.push_back(fsamHist);
    if (recHist == nullptr || edepHist == nullptr) {
      std::cerr << "failed to read histogram\n";
      return;
    }
    if (fsamHist == nullptr) {
      std::cerr << "failed to allocate TH1D\n";
      return;
    }
    for (size_t j = 0; j < energyBins.size(); ++j) {
      double rec = recHist->GetBinContent(j + 1);
      double recError = recHist->GetBinError(j + 1);
      double edep = edepHist->GetBinContent(j + 1);
      // double edepError = edepHist->GetBinError(j);
      double fsam;
      double fsamError;

      if (edep == 0.) {
        fsam = 0.;
        fsamError = recError;
      } else {
        fsam = rec / edep;
        fsamError = recError / edep;
      }
      fsamHist->SetBinContent(j + 1, fsam);
      fsamHist->SetBinError(j + 1, fsamError);
    }
    delete recHist;
    delete edepHist;
  }
  graph->SetTitle("Sampling fraction; Eta; Energy;");
  graph->SaveAs(fmt::format("{}fsam2Dgraph.root", resultPath).c_str());
  fsamFile->cd();
  for (TH1D* hist : fsamEHists) {
    hist->Write();
  }
  for (TH1D* hist : fsamEtaHists) {
    hist->Write();
  }
  fsamFile->Close();
}
