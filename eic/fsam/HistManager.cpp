// C++
#include <fmt/core.h>
#include <mutex>
#include <thread>
#include <utility>

// EDM
#include "edm4eic/CalorimeterHitCollectionData.h"
#include "edm4eic/ReconstructedParticleCollectionData.h"
#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/SimCalorimeterHitData.h"

#include "HistManager.hpp"

// vector of pairs of <column name, TH1D model>
static const std::vector<std::pair<std::string, ROOT::RDF::TH1DModel>>
  histTable{
    std::pair{
      "recEnergy",
      ROOT::RDF::TH1DModel{
        "recEnergy",
        "Reconstructed energy; Reconstructed energy [GeV]; Events",
        500,
        0.0,
        2.0 } },
    std::pair{
      "fsam",
      ROOT::RDF::TH1DModel{
        "fsam",
        "Sampling fraction; Sampling fraction; Events",
        400,
        0.0,
        0.2 } },
    std::pair{
      "simEnergy",
      ROOT::RDF::TH1DModel{
        "simEnergy",
        "Total deposited energy in BIC; Total deposited energy in BIC [GeV]; Events",
        500,
        0.0,
        20.0 } }
  };

double
convertGenEnergy(
  const std::vector<edm4eic::ReconstructedParticleData>& generatedParticles)
{
  double sum = 0.0;
  for (const auto& particle : generatedParticles) {
    sum += particle.energy;
  }
  return sum / generatedParticles.size(); // instead of generatedParticles[0]
};

double
convertRecEnergy(const std::vector<edm4eic::CalorimeterHitData>& event)
{
  const double eicrecon_fsam = 0.10200085;
  double sum = 0.0;
  for (const auto& hit : event) {
    sum += hit.energy;
  }
  return sum * eicrecon_fsam;
};

double
convertSimEnergy(const std::vector<edm4hep::SimCalorimeterHitData>& event)
{
  double sum = 0.0;
  for (const auto& hit : event) {
    sum += hit.energy;
  }
  return sum;
};

double
convertFsam(double recEnergy, double genEnergy)
{
  return recEnergy / genEnergy;
};

HistManager::HistManager(const std::string& pathPrefix, bool isSensitive)
  : m_pathPrefix(pathPrefix)
  , m_isSensitive(isSensitive)
  , m_energyBins(pathPrefix + "E_range")
  , m_etaBins(pathPrefix + "ETA_range")
{
  std::cout << "HistManager constructor begin\n";

  printBins();
  allocate();

  gStyle->SetOptFit(0);
  ROOT::EnableThreadSafety();
  EventHist::s_pathPrefix = m_pathPrefix;
  std::cout << "HistManager constructor end\n";
}

HistManager::~HistManager()
{
  delete m_fsam2DHist;
  delete m_recEnergy2DHist;
  delete m_simEnergy2DHist;
  /*
  for (auto& hist : m_energyHistArr) {
    delete hist;
  }
  for (auto& hist : m_etaHistArr) {
    delete hist;
  }
  */
  delete m_file;
}

void
HistManager::process()
{
  std::vector<std::thread> threadVec;

  /*
  auto work = [this]() {
    for (size_t energyBin = 0; energyBin < m_energyBins.size(); ++energyBin) {
      for (size_t etaBin = 0; etaBin < m_etaBins.size(); ++etaBin) {
        const std::string simInfo = fmt::format(
          "E{:.2f}_H{:.1f}t{:.1f}",
          m_energyBins[energyBin],
          m_etaBins.getLowerBound(etaBin),
          m_etaBins.getUpperBound(etaBin));
        ROOT::RDF::RNode dataNode = getDataNode(simInfo);
        fillHists(simInfo, energyBin, etaBin, dataNode);
      }
    }
  };
  work();
  */
  auto work = [this](size_t energyBin) {
    gStyle->SetOptFit(0);
    ROOT::EnableThreadSafety();
    std::cout << "thread " << energyBin << " begin\n";
    for (size_t etaBin = 0; etaBin < m_etaBins.size(); ++etaBin) {
      const std::string simInfo = fmt::format(
        "E{:.2f}_H{:.1f}t{:.1f}",
        m_energyBins[energyBin],
        m_etaBins.getLowerBound(etaBin),
        m_etaBins.getUpperBound(etaBin));
      ROOT::RDF::RNode dataNode = getDataNode(simInfo);
      fillHists(simInfo, energyBin, etaBin, dataNode);
    }
    std::cout << "thread " << energyBin << " end\n";
  };

  std::cout << "starting " << m_energyBins.size() << " threads\n";
  for (size_t energyBin = 0; energyBin < m_energyBins.size(); ++energyBin) {
    threadVec.emplace_back(work, energyBin);
  }
  for (size_t energyBin = 0; energyBin < m_energyBins.size(); ++energyBin) {
    threadVec[energyBin].join();
  }
}

void
HistManager::storeHists()
{
  if (m_isSensitive == false) {
    m_fsam2DHist->SetTitle("; Eta; Energy; Sampling fraction");
    m_fsam2DHist->SaveAs(fmt::format("{}fsam2Dgraph.root", m_pathPrefix).c_str());
    m_recEnergy2DHist->SetTitle("; Eta; Energy; Sum of reconstructed hits' energy");
    m_recEnergy2DHist->SaveAs(fmt::format("{}rec2Dgraph.root", m_pathPrefix).c_str());
  } else {
    m_simEnergy2DHist->SetTitle("; Eta; Energy; Calorimeter deposit energy");
    m_simEnergy2DHist->SaveAs(
      fmt::format("{}edep2Dgraph.root", m_pathPrefix).c_str());
  }
  m_file->cd();
  for (auto& hist : m_energyHistArr) {
    hist->Write();
  }
  for (auto& hist : m_etaHistArr) {
    hist->Write();
  }
  m_file->Close();
  std::cout << "result is written to ROOT file.\n";
}

void
HistManager::allocate()
{
  m_fsam2DHist = new TGraph2DErrors();
  m_recEnergy2DHist = new TGraph2DErrors();
  m_simEnergy2DHist = new TGraph2DErrors();
  if (m_fsam2DHist == nullptr || m_recEnergy2DHist == nullptr
      || m_simEnergy2DHist == nullptr) {
    throw std::runtime_error("failed to allocate TGraph2DErrors.");
  }
  std::cout << "TGraph2DErrors instances allocated\n";

  m_file =
    new TFile(fmt::format("{}1DHists.root", m_pathPrefix).c_str(), "CREATE");
  if (m_file == nullptr) {
    throw std::runtime_error("failed to create ROOT file.");
  }
  if (m_file->IsOpen() == kFALSE) {
    throw std::runtime_error("failed to open ROOT file.");
  }
  std::cout << "TFile instance allocated\n";

  for (size_t i = 0; i < m_energyBins.size(); ++i) {
    std::string histName = fmt::format("E{}", m_energyBins[i]);
    TH1D* hist = new TH1D(
      histName.c_str(),
      histName.c_str(),
      m_etaBins.size(),
      m_etaBins.getBinEdges());
    m_energyHistArr.push_back(hist);
  }
  for (size_t i = 0; i < m_etaBins.size(); ++i) {
    std::string histName = fmt::format("eta{}", m_etaBins.getMiddleValue(i));
    TH1D* hist = new TH1D(
      histName.c_str(),
      histName.c_str(),
      m_energyBins.size(),
      m_energyBins.getBinEdges());
    m_etaHistArr.push_back(hist);
  }
}

void
HistManager::printBins()
{
  m_etaBins.printBins();
  m_etaBins.printBinEdges();
  m_energyBins.printBins();
  m_energyBins.printBinEdges();
}

// get data nodes from ROOT file which has reconstructed results.
ROOT::RDF::RNode
HistManager::getDataNode(const std::string& simInfo)
{
  const TString inputName =
    fmt::format("{}/rec/rec_{}.root", m_pathPrefix, simInfo);

  // open a ROOT file containing simulated and reconstructed hits created by
  // eicrecon and get data frame.
  ROOT::RDataFrame dataFrame("events", inputName);

  // create new data node for generated energy,
  auto dataNode = ROOT::RDF::RNode(
    dataFrame.Define("genEnergy", convertGenEnergy, { "GeneratedParticles" }));

  // reconstructed energy and sampling fraction.
  dataNode =
    dataNode.Define("recEnergy", convertRecEnergy, { "EcalBarrelScFiRecHits" })
      .Define("fsam", convertFsam, { "recEnergy", "genEnergy" });

  // if 'EcalBarrelScFiHits' exists in the data frame,
  // it means that the ROOT file should have been generated from a simulation
  // where entire detector is sensitive detector
  // to get 'calorimeter' deposit energy but 'calorimeter sensor' energy
  // deposit.
  if (m_isSensitive == true) {
    dataNode =
      dataNode.Define("simEnergy", convertSimEnergy, { "EcalBarrelScFiHits" });
  }
  std::cout << "getDataNode end\n";
  return dataNode;
}

void
HistManager::fillHists(
  const std::string& simInfo,
  size_t energyBin,
  size_t etaBin,
  ROOT::RDF::RNode& dataNode)
{
  static std::mutex histMutex;
  // we have a data node which contains columns
  // for reconstructed energy, sampling fraction and optional simulated
  // energy. histograms will be generated from the columns.

  // EventHist class has two methods.
  // one: given a data node, calculate mean value from gaussian fit.
  // Data column for fitting is determined when it is initialized.
  // two: draw histogram to image file. not implemented yet.
  // EventHist recEHist(histTable[0].first, histTable[0].second, simInfo);

  if (m_isSensitive == false) {
    EventHist recEHist(histTable[0].first, histTable[0].second, simInfo);
    EventHist fsamHist(histTable[1].first, histTable[1].second, simInfo);
    auto fsamMean = fsamHist.getGausFitMean(dataNode, true);
    auto recEMean = recEHist.getGausFitMean(dataNode, true);

    histMutex.lock();
    m_energyHistArr[energyBin]->SetBinContent(etaBin + 1, recEMean.first);
    m_energyHistArr[energyBin]->SetBinError(etaBin + 1, recEMean.second);
    m_etaHistArr[etaBin]->SetBinContent(energyBin + 1, recEMean.first);
    m_etaHistArr[etaBin]->SetBinError(energyBin + 1, recEMean.second);
    m_fsam2DHist->SetPoint(
      energyBin * m_etaBins.size() + etaBin,
      m_etaBins.getMiddleValue(etaBin),
      m_energyBins[energyBin],
      fsamMean.first);
    m_fsam2DHist->SetPointError(
      energyBin * m_etaBins.size() + etaBin,
      m_etaBins.getMiddleValue(etaBin),
      m_energyBins[energyBin],
      fsamMean.second);
    m_recEnergy2DHist->SetPoint(
      energyBin * m_etaBins.size() + etaBin,
      m_etaBins.getMiddleValue(etaBin),
      m_energyBins[energyBin],
      recEMean.first);
    m_recEnergy2DHist->SetPointError(
      energyBin * m_etaBins.size() + etaBin,
      m_etaBins.getMiddleValue(etaBin),
      m_energyBins[energyBin],
      recEMean.second);
    histMutex.unlock();
  } else {
    EventHist simEHist(histTable[2].first, histTable[2].second, simInfo);
    auto simEMean = simEHist.getGausFitMean(dataNode, true);

    histMutex.lock();
    m_energyHistArr[energyBin]->SetBinContent(etaBin + 1, simEMean.first);
    m_energyHistArr[energyBin]->SetBinError(etaBin + 1, simEMean.second);
    m_etaHistArr[etaBin]->SetBinContent(energyBin + 1, simEMean.first);
    m_etaHistArr[etaBin]->SetBinError(energyBin + 1, simEMean.second);
    m_simEnergy2DHist->SetPoint(
      energyBin * m_etaBins.size() + etaBin,
      m_etaBins.getMiddleValue(etaBin),
      m_energyBins[energyBin],
      simEMean.first);
    m_simEnergy2DHist->SetPointError(
      energyBin * m_etaBins.size() + etaBin,
      m_etaBins.getMiddleValue(etaBin),
      m_energyBins[energyBin],
      simEMean.second);
    histMutex.unlock();
  }
  std::cout << "fillHists end\n";
}
