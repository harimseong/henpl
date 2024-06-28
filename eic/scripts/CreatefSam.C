#include "ROOT/RDataFrame.hxx"
#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "edm4eic/CalorimeterHitCollectionData.h"
#include "edm4eic/ReconstructedParticleCollectionData.h"
#include "edm4hep/MCParticleCollection.h"

#include "ROOT/TProcessExecutor.hxx"
#include "ROOT/TSeq.hxx"
#include "TCanvas.h"
#include "TF1.h"
#include "TH1.h"
#include "TH1D.h"
#include "TMath.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TTimeStamp.h"
#include <TGraph2D.h>
#include <TGraph2DErrors.h>
#include <nlohmann/json.hpp>

#include "emcal_barrel_common_functions.h"

#include "fSam.hpp"

using ROOT::RDataFrame;
using namespace ROOT::VecOps;
using json = nlohmann::json;

std::mutex stdDevMtx;
std::mutex histMtx;

static std::vector<std::string> path_strings = {"/usr/local/share/eic/results/electron_5000evt", "/usr/local/share/eic/results/photon_5000evt"};
static std::vector<std::string> particle_names = {"electron", "photon"};
static std::string save_path = "/usr/local/share/eic/results";
static std::string path_string;

// Function prototypes
void save_canvas(TCanvas *c, std::string label, std::string particle_label);
void save_canvas(TCanvas *c, const std::string &label);
int CreatefSamHist(std::string path_prefix, std::string particle_name);
std::vector<double> calculate_fSam_mean(const std::string &input_fname,
                                        const std::string &info);

void CreatefSam() {
  for (int i = 0; i < path_strings.size(); ++i) {
    path_string = path_strings[i];
    CreatefSamHist(path_strings[i], particle_names[i]);
  }
}

int CreatefSamHist(std::string path_prefix, std::string particle_name) {
  // Set the simulation parameters
  std::vector<double> E_starts;
  const std::vector<double> ETA_mins = {-1.7, -1.6, -1.5, -1, -0.5,
                                        0,    0.5,  1,    1.2};
  const std::vector<double> ETA_maxs = {-1.6, -1.5, -1,  -0.5, 0,
                                        0.5,  1,    1.2, 1.3};
  Double_t MeanStdDev = 0.0;
  Int_t n = 0;
  TTimeStamp ts;

  int nEHistos;
  int nEtaHistos = ETA_mins.size();

  if (particle_name == "photon") {
    E_starts = {0.1, 0.15, 0.2, 0.25, 0.3,  0.35, 0.4,  0.45, 0.5, 0.75,
                1,   1.5,  2,   2.5,  3,    4,    5,    6,    7,   8,
                9,   10,   11,  12,   13.5, 15,   16.5, 18};
    nEHistos = E_starts.size();
  } else {
    E_starts = {0.5, 0.6, 0.7, 0.8, 0.9, 1,  1.5, 2,  2.5,  3,  4,    5,
                6,   7,   8,   9,   10, 11,  12, 13.5, 15, 16.5, 18};
    nEHistos = E_starts.size();
  }

  std::vector<double> ETA_mids;
  for (Int_t i = 0; i < ETA_mins.size(); ++i) {
    ETA_mids.push_back((ETA_mins[i] + ETA_maxs[i]) / 2);
  }

  TH1F *EHistos[nEHistos];
  TH1F *EtaHistos[nEtaHistos];

  // ROOT::EnableThreadSafety();

  // create Tfile to fill histos
  std::filesystem::create_directories(
      fmt::format("{}/rec/results", path_prefix));
  TFile *Tfile =
      new TFile(Form("%s/rec/results/fsam_%s_%u_%u.root", path_prefix.c_str(),
                     particle_name.c_str(), ts.GetDate(), ts.GetTime()),
                "RECREATE");

  json results;

  TH3F *histfSamEEta =
      new TH3F("h3_e_eta_fsam", "h3_e_eta_fsam", ETA_mins.size(), 0.5,
               0.5 + ETA_mins.size(), ETA_mins.size(), 0.5,
               0.5 + ETA_mins.size(), 1000, 0, 1);

  TGraph2DErrors *tg2d = new TGraph2DErrors();

  gStyle->SetOptStat(0);

  auto etaLoop = [&](int i) {
    EHistos[i] = new TH1F(Form("h1_fSam_E%zu", i), Form("h1_fSam_E%zu", i),
                          nEtaHistos, 0.5, 0.5 + nEtaHistos);
    for (size_t j = 0; j < nEtaHistos; j++) {
      double ETA_min = ETA_mins[j];
      double ETA_max = ETA_maxs[j];

      std::string sim_info;

      sim_info =
          fmt::format("E{:.2f}_H{:.1f}t{:.1f}", E_starts[i], ETA_min, ETA_max);
      const std::string input_fname =
          fmt::format("{}/rec/rec_{}.root", path_prefix, sim_info);
      if (i == 0)
        cout << sim_info << " root_file_sucessfully_opened!!" << endl;

      auto calculate_result = calculate_fSam_mean(input_fname, sim_info);
      double fSam_Mean = calculate_result.at(0);
      double fSam_Mean_Err = calculate_result.at(1);
      MeanStdDev += fSam_Mean_Err;

      if (i == 0) {
        EtaHistos[j] = new TH1F(fmt::format("h1_fSam_Eta{}", j).c_str(),
                                fmt::format("h1_fSam_Eta{}", j).c_str(),
                                nEHistos, 0.5, 0.5 + nEHistos);
      }

      EHistos[i]->SetBinContent(j + 1, fSam_Mean);
      EHistos[i]->SetBinError(j + 1, fSam_Mean_Err);
      EtaHistos[j]->SetBinContent(i + 1, fSam_Mean);
      EtaHistos[j]->SetBinError(i + 1, fSam_Mean_Err);

      histfSamEEta->SetBinContent(i + 1, j + 1, fSam_Mean);
      histfSamEEta->SetBinError(i + 1, j + 1, fSam_Mean_Err);
      // histMtx.unlock();

      // ** TGraph2D **
      tg2d->SetPoint(i * nEtaHistos + j, E_starts[i],
                     (ETA_mins[j] + ETA_maxs[j]) / 2, fSam_Mean);
      tg2d->SetPointError(i * nEtaHistos + j, 0, 0, fSam_Mean_Err);
    }
    return 0;
  };

  /*
  // Multithreading
  UInt_t  nProcesses = E_starts.size();
  ROOT::TProcessExecutor pool(nProcesses);

  auto result = pool.Map(etaLoop, ROOT::TSeqI(nProcesses));
  */

  for (size_t idx = 0; idx < nEHistos; idx++) {
    etaLoop(idx);
  }

  tg2d->SetTitle("x axis: Energy, y axis: eta, z axis: sampling fraction");
  tg2d->Draw("surf1");

  tg2d->SaveAs(fmt::format("{}/{}_{}.root", save_path, particle_name, ts.GetDate()).c_str());

  cout << Form("%dth For loop has sucessfully iterated!", n++) << endl;

  histfSamEEta->Write();
  delete histfSamEEta;

  for (Int_t i = 0; i < nEHistos; i++) {
    EHistos[i]->Write();
  }
  for (Int_t i = 0; i < nEtaHistos; i++) {
    EtaHistos[i]->Write();
  }

  cout << Form("%dth Writing histos sucessfully done!", n++) << endl;

  for (int i = 0; i < nEHistos; i++) {
    delete EHistos[i];
  }
  for (int i = 0; i < nEtaHistos; i++) {
    delete EtaHistos[i];
  }

  cout << Form("%dth Deleting histos sucessfully done!", n++) << endl;

  Tfile->Close();
  delete Tfile;

  cout << Form("%dth Closing TFile sucessfully done!", n++) << endl;

  // canfSamE->cd();
  // histfSamE->SetMarkerSize(1);
  // histfSamE->SetMarkerStyle(8);
  // histfSamE->Draw("pe");
  // save_canvas(canfSamE,"fsamE");
  // canfSamEta->cd();
  // histfSamEta->SetMarkerSize(1);
  // histfSamEta->SetMarkerStyle(8);
  // histfSamEta->Draw("pe");
  // save_canvas(canfSamEta,"fsamEta");
  // Save the results to a JSON file
  // std::ofstream o("results/fSam_mean_distribution.json");
  // o << std::setw(4) << results << std::endl;

  return 0;
}

std::vector<double> calculate_fSam_mean(const std::string &input_fname,
                                        const std::string &sim_info) {
  double Ethr_mean = 0.0;
  double fSam_mean = 0.0;
  double fSam_img_mean = 0.0;
  double fSam_scfi_mean = 0.0;
  double fSam_mean_err = 0.0;
  double fSam_img_mean_err = 0.0;
  double fSam_scfi_mean_err = 0.0;
  std::vector<double> output_result;

  // Load the file and set up the ROOT DataFrame
  ROOT::RDataFrame d0("events", input_fname);

  // Environment Variables
  std::string detector_path = "";
  std::string detector_name = "athena";
  if (std::getenv("DETECTOR_PATH")) {
    detector_path = std::getenv("DETECTOR_PATH");
  }
  if (std::getenv("DETECTOR_CONFIG")) {
    detector_name = std::getenv("DETECTOR_CONFIG");
  }

  // Thrown Energy [GeV]
  /*
  auto Ethr = [](std::vector<edm4hep::MCParticleData> const& input) {
    auto p      = input[2];
    auto energy = TMath::Sqrt(p.momentum.x * p.momentum.x + p.momentum.y *
  p.momentum.y + p.momentum.z * p.momentum.z + p.mass * p.mass); return energy;
  };
  */
  // edm4eic::GeneratedParticles and edm4eic::ReconstructedParticles use same
  // data structure.
  auto Ethr = [](std::vector<edm4eic::ReconstructedParticleData> const
                     &generatedParticles) {
    double_t sum = 0;
    for (const auto &gp : generatedParticles) {
      sum += gp.energy;
    }
    return sum / generatedParticles.size();
  };

  // Number of hits
  // auto nhits = [](const std::vector<edm4hep::SimCalorimeterHitData>& evt) {
  auto nhits = [](const std::vector<edm4eic::CalorimeterHitData> &evt) {
    return (int)evt.size();
  };

  // Energy Reconstructedion [GeV]
  // auto Erec = [](const std::vector<edm4hep::SimCalorimeterHitData>& evt) {
  auto Erec = [](const std::vector<edm4eic::CalorimeterHitData> &evt) {
    const double eicrecon_fsam_estimate = 0.10200085;
    auto total_edep = 0.0;
    for (const auto &i : evt)
      total_edep += i.energy * eicrecon_fsam_estimate;
    return total_edep;
  };

  // Sampling fraction = Esampling / Ethrown
  auto fsam = [](const double sampled, const double thrown) {
    return sampled / thrown;
  };

  // Define variables
  // auto d1 = ROOT::RDF::RNode(d0.Define("Ethr", Ethr, {"MCParticles"}));
  auto d1 = ROOT::RDF::RNode(d0.Define("Ethr", Ethr, {"GeneratedParticles"}));

  // Define optional variables
  auto fsam_estimate = 1.0;
  if (d1.HasColumn("EcalBarrelScFiRecHits")) {
    d1 = d1
             .Define("nhits", nhits,
                     {"EcalBarrelScFiRecHits"}) // size
                                                //.Define("ErecImg", Erec,
                                                //{"EcalBarrelScFiRecHits"}) //
             .Define("ErecScFi", Erec, {"EcalBarrelScFiRecHits"}) //
             //.Define("Erec", "ErecScFi")
             //.Define("fsamImg", fsam, {"ErecScFi", "Ethr"})
             .Define("fsamScFi", fsam, {"ErecScFi", "Ethr"});
    //.Define("fsam", fsam, {"Erec", "Ethr"});
    fsam_estimate = 0.1;
  } else {
    d1 = d1.Define("nhits", nhits, {"EcalBarrelSciGlassHits"})
             .Define("Erec", Erec, {"EcalBarrelSciGlassHits"})
             .Define("fsam", fsam, {"Erec", "Ethr"});
    fsam_estimate = 1.0;
  }

  // Define Histograms
  auto Ethr_max = 25.0;
  auto hEthr = d1.Histo1D({fmt::format("{}_hEthr", sim_info).c_str(),
                           "Thrown Energy; Thrown Energy [GeV]; Events", 100,
                           0.0, Ethr_max},
                          "Ethr");
  auto hNhits = d1.Histo1D({fmt::format("{}_hNhits", sim_info).c_str(),
                            "Number of hits per events; Number of hits; Events",
                            100, 0.0, 2000.0},
                           "nhits");
  // auto hErec = d1.Histo1D(
  //   {"hErec", "Energy Reconstructed; Energy Reconstructed [GeV]; Events",
  //   500, 0.0, fsam_estimate * Ethr_max}, "Erec");
  // auto hfsam = d1.Histo1D(
  //   {"hfsam", "Sampling Fraction; Sampling Fraction; Events", 400, 0.0, 2.0 *
  //   fsam_estimate}, "fsam");

  // Define optional histograms
  ROOT::RDF::RResultPtr<::TH1D> hErecImg, hErecScFi, hfsamImg, hfsamScFi;
  if (d0.HasColumn("EcalBarrelScFiRecHits")) {
    hErecScFi =
        d1.Histo1D({fmt::format("{}_hErecScFi", sim_info).c_str(),
                    "Energy Reconstructed; Energy Reconstructed [GeV]; Events",
                    500, 0.0, fsam_estimate * Ethr_max},
                   "ErecScFi");
    hfsamScFi = d1.Histo1D({fmt::format("{}_hfsamScFi", sim_info).c_str(),
                            "Sampling Fraction; Sampling Fraction; Events", 400,
                            0.0, 2.0 * fsam_estimate},
                           "fsamScFi");
  }

  // addDetectorName(detector_name, hEthr.GetPtr());
  // addDetectorName(detector_name, hErec.GetPtr());
  // addDetectorName(detector_name, hErecScFi.GetPtr());
  // addDetectorName(detector_name, hfsam.GetPtr());
  // addDetectorName(detector_name, hfsamScFi.GetPtr());

  // Event Counts
  auto nevents_thrown = d1.Count();
  // std::cout << "Number of Thrown Events: " << (*nevents_thrown) << "\n";

  (void)*nevents_thrown;
  (void)*nevents_thrown;
  // Draw Histograms
  if (hEthr.IsReady()) {
    std::string canvas_name = fmt::format("{}_c1", sim_info);
    TCanvas *c1 =
        new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 700, 500);
    c1->SetLogy(1);
    auto h = hEthr->DrawCopy();
    Ethr_mean = h->GetMean();
    h->SetLineWidth(2);
    h->SetLineColor(kBlue);
    // save_canvas(c1, Form("%s_Ethr", sim_info.c_str()));
  }

  if (hNhits.IsReady()) {
    std::string canvas_name = fmt::format("{}_c2", sim_info);
    TCanvas *c2 =
        new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 700, 500);
    c2->SetLogy(1);
    auto h = hNhits->DrawCopy();
    h->SetLineWidth(2);
    h->SetLineColor(kBlue);
    // save_canvas(c2, Form("%s_nhits", sim_info.c_str()));
  }

  /*
  if (hErec.IsReady()) {
    std::string canvas_name = Form("%s_herec", sim_info.c_str());
    TCanvas* c3 = new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 700,
  500); c3->SetLogy(1); auto h = hErec->DrawCopy(); h->SetLineWidth(2);
    h->SetLineColor(kBlue);
    double up_fit   = h->GetMean() + 5 * h->GetStdDev();
    double down_fit = h->GetMean() - 5 * h->GetStdDev();
    h->GetXaxis()->SetRangeUser(0., up_fit);
    save_canvas(c3,Form("%s_Erec", sim_info.c_str()),particle_name);
  }
  */

  if (hErecScFi.IsReady()) {
    std::string canvas_name = fmt::format("{}_hErecScFi", sim_info);
    TCanvas *c3 =
        new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 700, 500);
    c3->SetLogy(1);
    auto h = hErecScFi->DrawCopy();
    h->SetLineWidth(2);
    h->SetLineColor(kBlue);
    double up_fit = h->GetMean() + 5 * h->GetStdDev();
    double down_fit = h->GetMean() - 5 * h->GetStdDev();
    h->GetXaxis()->SetRangeUser(0., up_fit);
    // save_canvas(c3, Form("%s_ErecScFi", sim_info.c_str()));
  }

  /*
  if (hfsamImg.IsReady()) {
    // TCanvas* c5 = new TCanvas("c5", "c5", 700, 500);
    // auto h = hfsamImg->DrawCopy();
    h->SetLineWidth(2);
    h->SetLineColor(kBlue);
    double up_fit   = h->GetMean() + 5 * h->GetStdDev();
    double down_fit = h->GetMean() - 5 * h->GetStdDev();
    h->Fit("gaus", "", "", down_fit, up_fit);
    h->GetXaxis()->SetRangeUser(down_fit, up_fit);
    TF1* gaus         = h->GetFunction("gaus");
    fSam_img_mean     = gaus->GetParameter(1);
    fSam_img_mean_err = gaus->GetParError(1);
    gaus->SetLineWidth(2);
    gaus->SetLineColor(kRed);
    save_canvas(c5,"fsamImg",particle_name);
  }
  */

  if (hfsamScFi.IsReady()) {
    std::string canvas_name = fmt::format("{}_fsamScFi", sim_info);
    TCanvas *c6 =
        new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 700, 500);
    auto h = hfsamScFi->DrawCopy();
    h->SetLineWidth(2);
    h->SetLineColor(kBlue);
    double up_fit = h->GetMean() + 5 * h->GetStdDev();
    double down_fit = h->GetMean() - 5 * h->GetStdDev();
    h->Fit("gaus", "", "", down_fit, up_fit);
    h->GetXaxis()->SetRangeUser(down_fit, up_fit);
    TF1 *gaus = h->GetFunction("gaus");
    fSam_scfi_mean = gaus->GetParameter(1);
    fSam_scfi_mean_err = gaus->GetParError(1);
    gaus->SetLineWidth(2);
    gaus->SetLineColor(kRed);
    save_canvas(c6, Form("%s_fsamScFi", sim_info.c_str()));
  }

  /*
  if (hfsam.IsReady()) {
    // TCanvas* c4 = new TCanvas("c4", "c4", 700, 500);
    auto h = hfsam->DrawCopy();
    // h->SetLineWidth(2);
    // h->SetLineColor(kBlue);
    double up_fit   = h->GetMean() + 5 * h->GetStdDev();
    double down_fit = h->GetMean() - 5 * h->GetStdDev();
    h->Fit("gaus", "", "", down_fit, up_fit);
    h->GetXaxis()->SetRangeUser(down_fit, up_fit);
    TF1* gaus = h->GetFunction("gaus");
    // fSam_mean = h->GetMean();
    // fSam_mean_err = h->GetStdDev();
    fSam_mean     = gaus->GetParameter(1);
    fSam_mean_err = gaus->GetParError(1);
    // gaus->SetLineWidth(2);
    // gaus->SetLineColor(kRed);
    // save_canvas(c1,"fsam",particle_name);
  }
  */
  // Define your analysis as before, calculate and return fSam_mean
  // For brevity, just return a dummy value here
  output_result.push_back(fSam_scfi_mean);
  output_result.push_back(fSam_scfi_mean_err);
  return output_result; // Placeholder: replace with actual calculation
}

void save_canvas(TCanvas *c, const std::string &label) {
  // c->SaveAs(fmt::format("{}/{}.png", path_string, label).c_str());
  c->SaveAs(fmt::format("{}/{}.pdf", path_string, label).c_str());
}

void save_canvas(TCanvas *c, std::string label, std::string particle_label) {
  std::string label_with_E = fmt::format("rec_{}_{}", particle_label, label);
  save_canvas(c, label_with_E);
}

