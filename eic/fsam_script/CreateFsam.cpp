#include <iostream>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "edm4eic/CalorimeterHitCollectionData.h"
#include "edm4eic/ReconstructedParticleCollectionData.h"
#include "edm4hep/MCParticleCollection.h"

#include "ROOT/RDataFrame.hxx"
#include "ROOT/TProcessExecutor.hxx"
#include "ROOT/TSeq.hxx"
#include "TCanvas.h"
#include "TF1.h"
#include "TH1.h"
#include "TH1D.h"
#include "TMath.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TGraph2D.h"
#include "TGraph2DErrors.h"

#include <fmt/core.h>

#include "fSam.hpp"

class HistElement {
  TGraph2DErrors* graph2d;
};

std::pair<double, double> getEnergies(TFile& recFile);

static std::mutex outputMutex;

void CreateFsam() {

  for (int i = 0; i < files.size(); ++i) {

    auto file = files[i];

    CreateFsamHist(
      std::get<0>(file), // path prefix
      std::get<2>(file) // particle name
    );
  }
}

void CreateFsamHist(
  const std::string& pathPrefix,
  const std::string& particleName,
) {
  // global options
  ROOT::EnableImplicitMT();
  // ROOT::EnableThreadSafety();
  gStyle->SetOptStat(0);

  // decide energy bins for a particle.
  std::vector<double>* energyBinsPtr;
  if (particle_name == "photon")
    energyBins = &photon_ebins;
  else if (particle_name == "electron")
    energyBins = &electron_ebins;
  else {
    std::cerr 
      << "invalid particle name: "
      << particleName
      << '\n';
    return;
  }
  const std::vector<double>& energyBins = *energyBinsPtr;
  const size_t nEnergyBins = energyBins.size();

  // set eta bins
  const std::vector<double> etaBins(ETABINS.begin(), ETABINS.end());
  const std::vector<double>& etaLows = ETA_mins;
  const std::vector<double>& etaHighs = ETA_maxs;
  const std::vector<double>& etaMids = ETA_mids;
  const size_t nEtaBins = etaLows.size();

  // initialize histogram
  constexpr unsigned int histSize = 128;
  TH1F* energyHists[histSize];
  TH1F* etaHists[histSize];
  if (histSize < nEnergyBins || histSize < nEtaBins) {
    std::cerr
      << "output bins size(" 
      << (histSize < nEnergyBins ? nEnergyBins : nEtaBins)
      << ") is larger than static array size(" << histSize << ").\n";
    return
  }
  for (size_t i = 0; i < nEnergyBins; ++i) {
    TString histName = fmt::format("fsam_E{}", energyBins[i]);
    energyHists[i] = new TH1F(histName, histName,
      nEnergyBins, EnergyBins[0], EnergyBins[0] + nEnergyBins);
  }
  for (size_t i = 0; i < nEtaBins; ++i) {
    TString histName = fmt::format("fsam_eta{}", etaBins[i]);
    etaHists[i] = new TH1F(histName, histName,
      nEtaBins, etaLows[0], etaLows[0] + nEtaBins);
  }

  // initialize 2d graph
  TGraph2DErrors* graph2D[3] = new TGraph2DErrors[3];

  // create root file where histograms will be stored.
  TFile* histFile = new TFile(fmt::format("{}/fsam_{}_{}_{}.root", 
        pathPrefix, particleName, fsamInit.date, fsaminit.time), "CREATE");

  // loop expressed as lambda function for multithreading in the future.
  auto etaLoop = [&](int i) {
    for (size_t j = 0; j < nEtaBins; ++j) {
      // eta range low limit, high limit
      double etaLow = etaLows[j];
      double etaHigh = etaHighs[j];

      // get file name before open
      const TString simInfo = fmt::format("E{:.2f}_H{:.1f}t{:.1f}",
          energyBins[i], etaLow, etaHigh);
      const TString inputName = fmt::format("{}/rec/rec_{}.root",
          pathPrefix, simInfo);

      // open a data file containing simulated and reconstructed hits 
      // created by eicrecon and plugin.
      TFile recFile(inputFileName, "READ");
      ROOT::RDataFrame dataFrame("events", inputFileName);

      // C++ lambda functions for RDataFrame function to process data columns
      auto genEnergy = 
      [](const std::vector<edm4eic::ReconstructedParticleData>& generatedParticles) {
        double sum = 0.0;
        for (const auto& particle: generatedParticles) {
          sum += particle.energy;
        }
        return sum / generatedParticles.size(); // instead of generatedParticles[0]
      };
      auto recEnergy =
      [](const std::vector<edm4eic::CalorimeterHitData>& event) {
        const double eicrecon_fsam = 0.10200085;
        double sum = 0.0;
        for (const auto& hit: event) {
          sum = event.energy;
        }
        return sum * eicrecon_fsam;
      };
      auto simEnergy =
      [](const std::vector<edm4hep::SimCalorimeterHitData>& event) {
        double sum = 0.0;
        for (const auto& hit: event) {
          sum += hit.energy;
        }
        return sum;
      };
      auto fsam =
      [](double recEnergy, double genEnergy) {
        return recEnergy / genEnergy;
      };

      // create new columns from the data
      // TODO: remove doSimEnergy after applying eicrecon plugin
      bool doSimEnergy = false;
      auto dataNode = ROOT::RDF::RNode(dataFrame.Define("genEnergy", genEnergy, {"GeneratedParticles"}));
      dataNode = dataNode
        .Define("recEnergy", recEnergy, {"EcalBarrelScFiRecHits"})
        .Define("fsam", fsam, {"recEnergy","genEnergy"});
      if (d1.HasColumn("EcalBarrelScFiHits")) {
        dataNode = dataNode
          .Define("simEnergy", simEnergy, {"EcalBarrelScFiHits"});
        doSimEnergy = true;
      }

      // create histograms from the new columns
      ROOT::RDF::RResultPtr<::TH1D> histGenEnergy, histRecEnergy, histSimEnergy, histFsam;

      double fsamEstimate = 0.1;
      histGenEnergy = dataNode.Histo1D(
        {
          fmt::format("{} genEnergy", sim_info).c_str(),
          "Thorwn Energy; Thrown Energy [GeV]; Events",
          100, 0.0, energyBins.back() + 8.0
        },
        "genEnergy"
      );
      histRecEnergy = dataNode.Histo1D(
        {
          fmt::format("{} recEnergy", sim_info).c_str(),
          "Reconstructed Energy; Energy Reconstructed [GeV]; Events",
          500, 0.0, fsamEstimate * EnergyBins.back() + 8.0
        },
        "recEnergy"
      );
      histFsam = dataNode.Histo1D(
        {
          fmt::format("{} fsam", sim_info).c_str(),
          "Sampling Fraction; Sampling Fraction; Events",
          400, 0.0, 2 * fsamEstimate),
        },
        "fsam"
      );

      if (doSimEnergy) {
        histSimEnergy = dataNode.Histo1D(
          {
            fmt::format("{} simEnergy", sim_info).c_str(),
            "Simulation Energy; Simulation Energy [GeV]; Events",
            500, 0.0, fsamEstimate * EnergyBins.back() + 8.0
          },
          "simEnergy"
        );
      }

      std::vector gausFitHists {
        histRecEnergy,
        histFsam,
      };
      if (doSimEnergy)
        histArray.push_back(histSimEnergy);
      std::vector<double> gausFitMean;
      std::vector<double> gausFitMeanError;

      for (auto& hist: gausFitHists) {
        double upFit = hist->GetMean() + 5 * hist->GetStdDev();
        double downFit = hist->GetMean() - 5 * hist->GetStdDev();
        hist->Fit("gause", "", "", downFit, upFit);
        hist->GexXaxis()->SetRangeUser(downFit, upFit);
        TF1* gaus = hist->GetFunction("gaus");
        gausFitMean.push_back(gaus->GetParameter(1));
        gausFitMeanError.push_back(gaus->GetParError(1));
      }

      energyHists[i]->SetBinContent(j + 1, gausFitMean[0]);
      energyHists[i]->SetBinError(j + 1, gausFitMeanError[0]);
      etaHists[j]->SetBinContent(i + 1, gausFitMean[0]);
      etaHists[j]->SetBinError(i + 1, gausFitMeanError[0]);

      // write 

      // start of multithreading critical section
      // outputMutex.lock();
      
      // fill 2d graph
      fsam2DGraph->SetBinContent(
        i * nEtaBins + j,
        energyBins[i],
        (etaLows[j] + etaHighs[j]) / 2,
        gausFitMean[0]
      );
      fsam2DGraph->SetPointError(
        i * nEtaBins + j,
        0,
        0,
        gausFitMeanError[0]
      );

      // write histograms to output file
      histFile->cd();
      histGenEnergy->Write();
      histRecEnergy->Write();
      if (doSimEnergy)
        histSimEnergy->Write();

      // outputMutex.unlock();

      // canvas output
      // may not work with multithreading on
      if (0) {
        if (histRecEnergy.IsReady()) {
          std::string canvas_name = fmt::format("{} recEnergy", sim_info);
          TCanvas *canvas = new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 700, 500);
          canvas->SetLogy(1);
          auto hist = histRecEnergy->DrawCopy();
          hist->SetLineWidth(2);
          hist->SetLineColor(kBlue);
        }
        if (histFsam.IsReady()) {
          std::string canvas_name = fmt::format("{} fsam", sim_info);
          TCanvas *canvas = new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 700, 500);
          canvas->SetLogy(1);
          auto hist = histFsam->DrawCopy();
          hist->SetLineWidth(2);
          hist->SetLineColor(kBlue);
        }
        if (doSimEnergy && histSimEnergy.IsReady()) {
          std::string canvas_name = fmt::format("{} simEnergy", sim_info);
          TCanvas *canvas = new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 700, 500);
          canvas->SetLogy(1);
          auto hist = histSimEnergy->DrawCopy();
          hist->SetLineWidth(2);
          hist->SetLineColor(kBlue);
        }
      }
    }
  }

  /*
  // multithreading
  unsigned int nProcesses = nEnergyBins;
  ROOT::TProcessExecutor pool(nProcesses);

  auto result = pool.Map(etaLoop, ROOT::TSeqI(nProcesses));
  */
  for (size_t i = 0; i < nEnergyBins; ++i) {
    etaLoop(i);
  }
}

std::pair<double, double> getEnergies(TFile& recFile)
{
  ROOT::RDataFrame dataFrame();
}
