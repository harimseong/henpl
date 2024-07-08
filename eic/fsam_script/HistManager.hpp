#ifndef HISTMANAGER_HPP
#define HISTMANAGER_HPP

#include <string>
#include <vector>

#include "Eta.hpp"
#include "Energy.hpp"
#include "Hist.hpp"
#include "TTimeStamp.h"

class	HistManager
{

public:
	HistManager(const std::string& pathPrefix) 
  : m_pathPrefix(pathPrefix) {
    TTimStamp ts;

    m_date = ts.GetDate();
    m_time = ts.GetTime();

    initHists();
  }
	~HistManager() = default;
	HistManager(const HistManager& histmanager) = delete;
	HistManager	&operator=(const HistManager& histmanager) = delete;

  void initHists() {
  }

  void fillRDFFromROOT(double energyBin, double etaLow, double etaHigh) {
    const TString simInfo = fmt::format("E{:.2f}_H{:.1f}t{:.1f}",
        energyBin, etaLow, etaHigh);
    const TString inputName = fmt::format("{}/rec/rec_{}.root",
        m_pathPrefix, simInfo);

    // open a data file containing simulated and reconstructed hits 
    // created by eicrecon and plugin.
    ROOT::RDataFrame dataFrame("events", inputFileName);

    bool doSimEnergy = false;
    auto dataNode = ROOT::RDF::RNode(dataFrame.Define("genEnergy", convertGenEnergy, {"GeneratedParticles"}));
    dataNode = dataNode
      .Define("recEnergy", convertRecEnergy, {"EcalBarrelScFiRecHits"})
      .Define("fsam", convertFsam, {"recEnergy","genEnergy"});
    if (d1.HasColumn("EcalBarrelScFiHits")) {
      dataNode = dataNode
        .Define("simEnergy", convertSimEnergy, {"EcalBarrelScFiHits"});
      doSimEnergy = true;
    }

    // create histograms from the new columns
    ROOT::RDF::RResultPtr<::TH1D> histGenEnergy, histRecEnergy, histSimEnergy, histFsam;
  }

  void setHistsFromRDF(ROOT::RDF::RNode& dataNode) {
    for (auto& hist: m_histArr) {
      hist.setHist1D(dataNode);
    }
  }
  void setGraph2DPoints() {
    for (auto& hist: m_histArr) {
      auto meanerror = hist.getGausFitMean();
      hist.setPoint(meanerror.first);
      hist.setPointError(meanerror.second);
    }
    return ret;
  }

private:
  double convertGenEnergy(const std::vector<edm4eic::ReconstructedParticleData>& generatedParticles) {
    double sum = 0.0;
    for (const auto& particle: generatedParticles) {
      sum += particle.energy;
    }
    return sum / generatedParticles.size(); // instead of generatedParticles[0]
  };
  double convertRecEnergy(const std::vector<edm4eic::CalorimeterHitData>& event) {
    const double eicrecon_fsam = 0.10200085;
    double sum = 0.0;
    for (const auto& hit: event) {
      sum = event.energy;
    }
    return sum * eicrecon_fsam;
  };
  double convertSimEnergy(const std::vector<edm4hep::SimCalorimeterHitData>& event) {
    double sum = 0.0;
    for (const auto& hit: event) {
      sum += hit.energy;
    }
    return sum;
  };
  double convertFsam(double recEnergy, double genEnergy) {
    return recEnergy / genEnergy;
  };

private:
  const std::string m_pathPrefix;
  std::vector<Hist> m_histArr;
  unsigned int m_date;
  unsigned int m_time;
  Energy  m_energy;
  Eta     m_eta;

};

#endif // HISTMANAGER_HPP
