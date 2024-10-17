#ifndef HISTMANAGER_HPP
#define HISTMANAGER_HPP

// C++
#include <string>
#include <vector>

// ROOT
#include "ROOT/RDataFrame.hxx"
#include "TGraph2DErrors.h"
#include "TH1D.h"
#include "TStyle.h"

// headers
#include "Energy.hpp"
#include "Eta.hpp"
#include "EventHist.hpp"

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
 *   1. a TGraph2DErrors and TH1D histograms
 */
class HistManager
{
public:
  HistManager(const std::string& pathPrefix, bool isSensitive);
  ~HistManager();

  HistManager(const HistManager& histmanager) = delete;
  HistManager& operator=(const HistManager& histmanager) = delete;

  void process();
  void storeHists();

private:
  void allocate();
  void printBins();

  ROOT::RDF::RNode getDataNode(const std::string& simInfo);
  void fillHists(
    const std::string& simInfo,
    size_t energyBin,
    size_t etaBin,
    ROOT::RDF::RNode& dataNode);

private:
  TFile* m_file;
  const std::string m_pathPrefix;

  TGraph2DErrors* m_fsam2DHist;
  TGraph2DErrors* m_recEnergy2DHist;
  TGraph2DErrors* m_simEnergy2DHist;
  std::vector<TH1D*> m_energyHistArr;
  std::vector<TH1D*> m_etaHistArr;

  bool m_isSensitive;
  const Energy m_energyBins;
  const Eta m_etaBins;
};

#endif // HISTMANAGER_HPP
