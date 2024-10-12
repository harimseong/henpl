#ifndef HISTMANAGER_HPP
#define HISTMANAGER_HPP

// C++
#include <fmt/core.h>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// EDM
#include "edm4eic/CalorimeterHitCollectionData.h"
#include "edm4eic/ReconstructedParticleCollectionData.h"
#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/SimCalorimeterHitData.h"

// ROOT
#include "ROOT/RDataFrame.hxx"
#include "TGraph2DErrors.h"
#include "TH1D.h"

// headers
#include "Energy.hpp"
#include "Eta.hpp"
#include "EventHist.hpp"

static constexpr double fsamEstimate = 0.1;

// vector of pairs of <column name, TH1D model>
static const std::vector<std::pair<std::string, ROOT::RDF::TH1DModel>>
  histTable{
    std::pair{
      "recEnergy",
      ROOT::RDF::TH1DModel{
        "recEnergy",
        "Reconstructed Energy; Energy Reconstructed [GeV]; Events",
        500,
        0.0,
        fsamEstimate * 20.0 } },
    std::pair{
      "fsam",
      ROOT::RDF::TH1DModel{
        "fsam",
        "Sampling Fraction; Sampling Fraction; Events",
        400,
        0.0,
        fsamEstimate * 2.0 } },
    std::pair{
      "simEnergy",
      ROOT::RDF::TH1DModel{
        "simEnergy",
        "Simulation Energy; Simulation Energy [GeV]; Events",
        500,
        0.0,
        fsamEstimate * 20.0 } }
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

/*
 * Input:
 *   1. A set of ROOT files that varies by energy and eta.
 *   2. energy list and eta list.
 *
 * Process:
 *   1. select a pair of energy and eta from the list.
 *   2. get a ROOT file by them.
 *   3. extract data nodes from the ROOT file.
 *   4. calculate sampling fraction using the data nodes.
 *   5. save the value to 2D graph, 1D Eta histogram and 1D Energy histogram.
 *
 * Output:
 *   1. ROOT file containing TGraph2DErrors, ...
 */
class HistManager
{
public:
  HistManager(const std::string& pathPrefix)
    : m_pathPrefix(pathPrefix)
    , m_isSimEnergy(false)
    , m_energyBins(pathPrefix + "E_range")
    , m_etaBins(pathPrefix + "ETA_range")
  {
    std::cout << "HistManager constructor begin\n";
    auto tuple = parsePathprefix(pathPrefix);

    m_particle = std::get<0>(tuple);
    m_date = std::get<1>(tuple);
    m_time = std::get<2>(tuple);

    allocate();
    printBins();

    ROOT::EnableThreadSafety();
    EventHist::s_pathPrefix = m_pathPrefix;
    std::cout << "HistManager constructor end\n";
  }

  ~HistManager()
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

  HistManager(const HistManager& histmanager) = delete;
  HistManager& operator=(const HistManager& histmanager) = delete;

  void process()
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
          ROOT::RDF::RNode dataNode = getDataNode(sinInfo);
          fillHists(simInfo, energyBin, etaBin, dataNode);
        }
      }
    };
    work();
    */
    auto work = [this](size_t energyBin) {
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

  void storeHists()
  {
    m_file->cd();
    m_fsam2DHist->Write();
    // m_recEnergy2DHist->Write();
    // m_simEnergy2DHist->Write();
    for (auto& hist : m_energyHistArr) {
      hist->Write();
    }
    for (auto& hist : m_etaHistArr) {
      hist->Write();
    }
    m_file->Close();
    std::cout << "result is written to ROOT file.\n";
  }

private:
  void allocate()
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
      new TFile(fmt::format("{}fsam.root", m_pathPrefix).c_str(), "CREATE");
    if (m_file == nullptr) {
      throw std::runtime_error("failed to create ROOT file.");
    }
    if (m_file->IsOpen() == kFALSE) {
      throw std::runtime_error("failed to open ROOT file.");
    }
    std::cout << "TFile instance allocated\n";

    for (size_t i = 0; i < m_energyBins.size(); ++i) {
      std::string histName = fmt::format("fsam_E{}", m_energyBins[i]);
      TH1D* hist = new TH1D(
        histName.c_str(),
        histName.c_str(),
        m_etaBins.size(),
        0.5,
        0.5 + m_energyBins.size());
      m_energyHistArr.push_back(hist);
    }
    for (size_t i = 0; i < m_etaBins.size(); ++i) {
      std::string histName =
        fmt::format("fsam_eta{}", m_etaBins.getMiddleValue(i));
      TH1D* hist = new TH1D(
        histName.c_str(),
        histName.c_str(),
        m_energyBins.size(),
        0.5,
        0.5 + m_etaBins.size());
      m_etaHistArr.push_back(hist);
    }
  }

  void printBins()
  {
    std::cout << "\neta bins:\n";
    for (size_t i = 0; i < m_etaBins.size(); ++i) {
      std::cout << m_etaBins.getMiddleValue(i) << '\n';
    }
    std::cout << "\nenergy bins:\n";
    for (size_t i = 0; i < m_energyBins.size(); ++i) {
      std::cout << m_energyBins[i] << '\n';
    }
    std::cout << '\n';
  }

  // get data nodes from ROOT file which has reconstructed results.
  ROOT::RDF::RNode getDataNode(const std::string& simInfo)
  {
    const TString inputName =
      fmt::format("{}/rec/rec_{}.root", m_pathPrefix, simInfo);

    // open a ROOT file containing simulated and reconstructed hits created by
    // eicrecon and get data frame.
    ROOT::RDataFrame dataFrame("events", inputName);

    // create new data node for generated energy,
    auto dataNode = ROOT::RDF::RNode(dataFrame.Define(
      "genEnergy", convertGenEnergy, { "GeneratedParticles" }));

    // reconstructed energy and sampling fraction.
    dataNode =
      dataNode
        .Define("recEnergy", convertRecEnergy, { "EcalBarrelScFiRecHits" })
        .Define("fsam", convertFsam, { "recEnergy", "genEnergy" });

    // if 'EcalBarrelScFiHits' exists in the data frame,
    // it means that the ROOT file should have been generated from a simulation
    // where entire detector is sensitive detector
    // to get 'calorimeter' deposit energy but 'calorimeter sensor' energy
    // deposit.
    if (dataNode.HasColumn("EcalBarrelScFiHits")) {
      dataNode = dataNode.Define(
        "simEnergy", convertSimEnergy, { "EcalBarrelScFiHits" });
      m_isSimEnergy = true;
    }
    std::cout << "getDataNode end\n";
    return dataNode;
  }

  void fillHists(
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
    EventHist fsamHist(histTable[1].first, histTable[1].second, simInfo);
    EventHist simEHist(histTable[2].first, histTable[2].second, simInfo);

    auto fsamMean = fsamHist.getGausFitMean(dataNode, true);
    // auto recEMean = recEHist.getGausFitMean(dataNode);

    histMutex.lock();
    std::cout << "____________________________________________________________\n";
    std::cout << "fsamMean = " << fsamMean.first;
    std::cout << "energyBin = " << energyBin << ", etaBin = " << etaBin << '\n';
    std::cout << "____________________________________________________________\n";
    m_energyHistArr[energyBin]->SetBinContent(etaBin + 1, fsamMean.first);
    m_energyHistArr[energyBin]->SetBinError(etaBin + 1, fsamMean.second);
    m_etaHistArr[etaBin]->SetBinContent(energyBin + 1, fsamMean.first);
    m_etaHistArr[etaBin]->SetBinError(energyBin + 1, fsamMean.second);
    std::cout << "TH1D bin set\n";
    m_fsam2DHist->SetPoint(
      energyBin * m_etaBins.size() + etaBin,
      m_energyBins[energyBin],
      m_etaBins.getMiddleValue(etaBin),
      fsamMean.first);
    /*
    m_fsam2DHist->SetPointError(
      energyBin * m_etaBins.size() + etaBin,
      0,
      0,
      fsamMean.second);
    */
    /*
    std::cout << "TGraph2DErrors set 1\n";
    m_recEnergy2DHist->SetPoint(
      energyBin * m_etaBins.size() + etaBin,
      m_energyBins[energyBin],
      m_etaBins.getMiddleValue(etaBin),
      recEMean.first);
    m_recEnergy2DHist->SetPointError(
      energyBin * m_etaBins.size() + etaBin,
      0,
      0,
      recEMean.second);
    */
    std::cout << "TGraph2DErrors set 2\n";
    if (false && m_isSimEnergy == true) {
      auto simEMean = simEHist.getGausFitMean(dataNode);
      m_simEnergy2DHist->SetPoint(
        energyBin * m_etaBins.size() + etaBin,
        m_energyBins[energyBin],
        m_etaBins.getMiddleValue(etaBin),
        simEMean.first);
      m_simEnergy2DHist->SetPointError(
        energyBin * m_etaBins.size() + etaBin,
        m_energyBins[energyBin],
        m_etaBins.getMiddleValue(etaBin),
        simEMean.second);
      std::cout << "TGraph2DErrors set 3\n";
    }
    histMutex.unlock();
    std::cout << "fillHists end\n";
  }

private:
  TFile* m_file;
  const std::string m_pathPrefix;

  TGraph2DErrors* m_fsam2DHist;
  TGraph2DErrors* m_recEnergy2DHist;
  TGraph2DErrors* m_simEnergy2DHist;
  std::vector<TH1D*> m_energyHistArr;
  std::vector<TH1D*> m_etaHistArr;

  bool m_isSimEnergy; // for backward compatibility
  std::string m_particle;
  unsigned int m_date;
  unsigned int m_time;

  const Energy m_energyBins;
  const Eta m_etaBins;
};

#endif // HISTMANAGER_HPP
