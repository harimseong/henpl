#include <TCanvas.h>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TMath.h>
#include <TStyle.h>
#include <TTimeStamp.h>

#include <array>
#include <iostream>
#include <sstream>
#include <vector>

// 128 is arbitrary value for size of static array.
constexpr unsigned int max_ebin = 128;
constexpr unsigned int max_etabin = 128;

const std::vector<Double_t> electron_ebins =
{
    0.5,  0.6,  0.7,  0.8, 
    0.9,  1.0,  1.5,  2.0,
    2.5,  3.0,  4.0,  5.0,
    6.0,  7.0,  8.0,  9.0,
   10.0, 11.0, 12.0, 13.5,
   15.0, 16.5, 18.0
};

const std::vector<Double_t> photon_ebins =
{
    0.1, 0.15, 0.20, 0.25,
    0.3, 0.35, 0.40, 0.45,
    0.5, 0.75,  1.0,  1.5,
    2.0,  2.5,  3.0,  4.0,
    5.0,  6.0,  7.0,  8.0,
    9.0, 10.0, 11.0, 12.0,
   13.5, 15.0, 16.5, 18.0
};

constexpr std::array ETABINS{-1.7, -1.6, -1.5, -1.0, -0.5,
                             0.0,  0.5,  1.0,  1.2,  1.3};

constexpr Int_t ETAn = ETABINS.size() - 1;

// TODO: use ETABINS + ETABINS_end / 2 instead of ETAx
constexpr Double_t ETAx[ETAn] = {-1.65, -1.55, -1.25, -0.75, -0.25,
                                 0.25,  0.75,  1.1,   1.25};
constexpr Double_t ETAex[ETAn] = {0.05, 0.05, 0.25, 0.25, 0.25,
                                  0.25, 0.25, 0.1,  0.05};

Int_t Nevts = 5000;
const std::vector<std::pair<TString, TString>> files = {
    {"/usr/local/share/eic/results/electron_5000evt/rec/results/"
     "fsam_electron_20240612_152439.root",
     "electron"},
    // TODO: use updated root
    {"/usr/local/share/eic/results/photon_5000evt/rec/results/"
     "fsam_photon_20240603_61015.root",
     "photon"}};

const std::string savePath = "/usr/local/share/eic/results";

void setpad(TVirtualPad *pad, Double_t top = 0.15, Double_t left = 0.17, Double_t right = 0.00, Double_t bottom = 0.15);
void hset(TGraphErrors &hid, TString xtit = "", TString ytit = "",
          double titoffx = 1.0, double titoffy = 0.9, double titsizex = 0.08,
          double titsizey = 0.08, double labeloffx = 0.01,
          double labeloffy = 0.01, double labelsizex = 0.07,
          double labelsizey = 0.07, int divx = 510, int divy = 510);
TF1 *CreateFitFunc(const TString &tag);
TString GetFormular(TF1 *tformula);

std::vector<int> ColorPallete = {
    kRed,         kBlue,       kGreen + 1, kMagenta + 1, kCyan + 1,
    kOrange + 1,  kYellow + 2, kAzure + 7, kViolet + 1,  kSpring - 6,
    kPink + 7,    kTeal + 3,   kAzure + 2, kOrange - 3,  kSpring + 8,
    kMagenta - 3, kYellow - 3, kRed - 4,   kGreen - 5,   kBlue - 6};

TTimeStamp ts;

/*
 * @fileName: path to podIO file
 * @tag: particle name
 */
void fSam(const TString &fileName, const TString &tag) {
  TFile *file = TFile::Open(fileName, "READ");
  if (!file || file->IsZombie()) {
    std::cerr << "Cannot open file: " << fileName << std::endl;
    return;
  }

  const Double_t *Ex;
  Int_t En;
  std::string particle = tag.Data();
  if (particle == "electron") {
    Ex = electron_ebins.data();
    En = electron_ebins.size();
  } else if (particle == "photon") {
    Ex = photon_ebins.data();
    En = photon_ebins.size();
  } else {
    std::cerr << "invalid particle name: " << fileName << ": " << particle
              << '\n';
    return;
  }

  // energy bin

  gStyle->SetOptFit(0); // gStyle->SetOptFit(1011);
  gStyle->SetTitleFontSize(0.1);
  gStyle->SetTitleAlign(23);
  gStyle->SetStatFontSize(0.02);

  TCanvas *canvasE = new TCanvas("canvasE", "GraphsE in 3x3 Grid", 1200, 900);
  canvasE->Divide(3, 3);

  TCanvas *ratioCanvas =
      new TCanvas("ratioCanvas_Eta", "Ratio Plot for Eta", 1200, 900);
  ratioCanvas->Divide(3, 3);

  TCanvas *fitParamCanvas =
      new TCanvas("paramCanvas_Eta", "Fit Parameters Plot for Eta", 600, 600);

  TF1 *fitFunc = CreateFitFunc(tag);
  fitFunc->SetLineWidth(1);
  fitFunc->SetRange(0, 2 * Ex[En - 1] - Ex[En - 2]);

  std::vector<TGraphErrors *> fitParamGraphs;
  for (int i = 0; i < fitFunc->GetNpar(); ++i) {
    fitParamGraphs.push_back(new TGraphErrors(ETAn));
    fitParamGraphs.back()->SetTitle(Form("p%d", i));
  }
  fitParamCanvas->Divide(fitFunc->GetNpar() / 3, 3);

  for (int eta = 0; eta < ETAn; ++eta) {
    canvasE->cd(eta + 1);
    auto pad = canvasE->GetPad(eta + 1);
    setpad(pad);
    TLegend *legendEta = new TLegend(0.25, 0.2, 0.8, 0.45);
    legendEta->SetBorderSize(0);  // 범례 테두리 제거
    legendEta->SetTextSize(0.06); // 범례 텍스트 크기 설정
    TString histName = Form("h1_fSam_Eta%d", eta);
    TH1 *hist = dynamic_cast<TH1 *>(file->Get(histName));
    if (!hist) {
      std::cerr << "Histogram not found: " << histName << std::endl;
      continue;
    }

    Double_t Ey[max_ebin] = {};
    Double_t Eey[max_ebin] = {};

    for (int i = 1; i <= hist->GetNbinsX(); ++i) {
      Ey[i - 1] = hist->GetBinContent(i);
      Eey[i - 1] = hist->GetBinError(i);
    }

    TGraphErrors *graph = new TGraphErrors(En, Ex, Ey, nullptr, Eey);

    const char *PadTitle = Form(
        "%s, N_{evt}: %i, N_{hit}/N_{evt} ~ #it{O}(1e3)", tag.Data(), Nevts);
    graph->SetTitle(PadTitle);
    graph->SetMarkerStyle(kFullCircle);
    graph->SetMarkerSize(.9);
    // graph->SetLineWidth(.1);
    // graph->SetLineColor(ColorPallete[eta % ColorPallete.size()]); // 각
    // eta별로 팔레트 색상 설정 graph->SetMarkerColor(ColorPallete[eta %
    // ColorPallete.size()]);
    graph->SetMarkerColor(kBlack);
    hset(*graph, "#it{E} (GeV)", "f_{#it{s}}");
    graph->GetYaxis()->SetTitleFont(62); 
    graph->GetYaxis()->RotateTitle(0);

    graph->Draw(eta == 0 ? "APE" : "APE SAME");
    legendEta->AddEntry(
        graph, Form("#eta: %.1f < #eta < %.1f", ETABINS[eta], ETABINS[eta + 1]),
        "PE");

    graph->GetYaxis()->SetRangeUser(0.05, 0.12);
    graph->Fit(fitFunc, "RB");

    legendEta->AddEntry(fitFunc, GetFormular(fitFunc), "L");
    legendEta->Draw();

    // Ratio plot
    ratioCanvas->cd(eta + 1);
    auto padratio = ratioCanvas->GetPad(eta + 1);
    setpad(padratio);
    TLegend *legendEtaRat = new TLegend(0.25, 0.2, 0.9, 0.45);
    legendEtaRat->SetBorderSize(0); // 범례 테두리 제거
    legendEtaRat->SetTextSize(0.06);

    Double_t ratioY[max_ebin];
    Double_t ratioEY[max_ebin];

    for (int i = 0; i < En; ++i) {
      Double_t fitValue = fitFunc->Eval(Ex[i]);
      if (fitValue != 0) {
        ratioY[i] = Ey[i] / fitValue;
        ratioEY[i] = Eey[i] / fitValue;
      } else {
        ratioY[i] = 0;
        ratioEY[i] = 0;
      }
    }

    // ratio between values and fitting function
    TGraphErrors *ratioGraph =
        new TGraphErrors(En, Ex, ratioY, nullptr, ratioEY);
    ratioGraph->SetTitle(PadTitle);
    ratioGraph->SetMarkerStyle(kFullCircle);
    ratioGraph->SetMarkerSize(.7);
    ratioGraph->SetMarkerColor(kRed);
    ratioGraph->SetLineColor(kRed);

    hset(*ratioGraph, "#it{E} (GeV)", "EICrec / Fit");
    ratioGraph->GetYaxis()->SetRangeUser(0.8, 1.2);
    ratioGraph->Draw(eta == 0 ? "APE" : "APE SAME");

    TLine *line =
        new TLine(2 * Ex[0] - Ex[1], 1, 2 * Ex[En - 1] - Ex[En - 2], 1);
    line->SetLineColor(kBlack);
    line->SetLineStyle(3);
    line->SetLineWidth(1);
    line->Draw("same");

    legendEtaRat->AddEntry(
        ratioGraph,
        Form("#eta: %.1f < #eta < %.1f", ETABINS[eta], ETABINS[eta + 1]), "");
    legendEtaRat->AddEntry(fitFunc, GetFormular(fitFunc), "");
    legendEtaRat->Draw();

    // store fit parameters
    for (int i = 0; i < fitFunc->GetNpar(); ++i) {
      fitParamGraphs[i]->SetPoint(eta, (ETABINS[eta] + ETABINS[eta + 1]) / 2, fitFunc->GetParameter(i));
    }
  }

  // TMultiGraph* mg = new TMultiGraph();
  for (int i = 0; i < fitFunc->GetNpar(); ++i) {
    fitParamCanvas->cd(i + 1);
    auto padpar = fitParamCanvas->GetPad(i + 1);
    padpar->SetGrid();
    setpad(padpar, 0.15, 0.1, 0, 0.2);
    fitParamGraphs[i]->SetMarkerColor(kBlack);
    fitParamGraphs[i]->SetMarkerSize(.5);
    fitParamGraphs[i]->SetMarkerStyle(20);
    fitParamGraphs[i]->SetLineColor(kBlack);
    // mg->Add(fitParamGraphs[i]);
    hset(*fitParamGraphs[i], "#eta", "", 1, 1, 0.1, 0.1, 0.03, 0.01, 0.07, 0.07, 510, 510 );
    fitParamGraphs[i]->Draw();
  }
  // mg->Draw("p");

  ratioCanvas->SaveAs(Form("%s/RatioPlot_%s_Eta.pdf", savePath.c_str(), tag.Data()));
  delete ratioCanvas;
  canvasE->SaveAs(Form("%s/h1_%s_fSam_Etas.pdf", savePath.c_str(), tag.Data()));
  delete canvasE;
  fitParamCanvas->SaveAs(Form("%s/fitParamGraphs_%s_Eta.pdf", savePath.c_str(), tag.Data()));
  delete fitParamCanvas;

  TCanvas *canvasEta =
      new TCanvas("canvasEta", "GraphsEta in 4x5 Grid", 1200, 900);
  canvasEta->Divide(4, 5);
  for (int EBIN_idx = 0; EBIN_idx < En; ++EBIN_idx) {
    // TCanvas* cE      = new TCanvas("cE", "h1_fSam_Energies", 800, 600);
    canvasEta->cd(EBIN_idx + 1);
    TLegend *legendE = new TLegend(0.6, 0.1, 0.8, 0.5);
    legendE->SetBorderSize(0);  // 범례 테두리 제거
    legendE->SetTextSize(0.03); // 범례 텍스트 크기 설정
    TString histName = Form("h1_fSam_E%d", EBIN_idx);
    TH1 *hist = dynamic_cast<TH1 *>(file->Get(histName));
    if (!hist) {
      std::cerr << "Histogram not found: " << histName << std::endl;
      continue;
    }

    Double_t ETAy[ETAn] = {};
    Double_t ETAey[ETAn] = {};

    std::cout << "hist->GetNbinsX() : " << hist->GetNbinsX() << std::endl;
    for (int i = 1; i <= hist->GetNbinsX(); ++i) {
      ETAy[i - 1] = hist->GetBinContent(i);
      ETAey[i - 1] = hist->GetBinError(i);
    }
    TGraphErrors *graph = new TGraphErrors(ETAn, ETAx, ETAy, ETAex, ETAey);

    graph->SetMarkerSize(1);
    graph->SetLineColor(
        ColorPallete[EBIN_idx %
                     ColorPallete.size()]); // 각 Energy별로 팔레트 색상 설정
    graph->SetMarkerColor(ColorPallete[EBIN_idx % ColorPallete.size()]);

    graph->Draw(EBIN_idx == 0 ? "APEL" : "APEL SAME");
    legendE->AddEntry(graph, Form("E: %.1f GeV", Ex[EBIN_idx]), "lp");

    graph->GetYaxis()->SetRangeUser(0.07, 0.10);

    // TF1* fitFunc = new TF1("exponential_fit", " [0]/(TMath::Exp(([1] -
    // x)/[2]) + 1.0)", -1.7, 1.3); TF1* fitFunc = new TF1("exponential_fit",
    // "[0]/([1] + TMath::Exp([2]*x-[3]))", -1.7, 1.3);
    TF1 *fitFunc = new TF1("polynomial_fits", "pol2", -1.7, 1.3);
    // Tfunction으로 initial parameter

    graph->Fit(fitFunc, "RB");
    legendE->Draw();
  }
  canvasEta->SaveAs(Form("%s/h1_%s_fSam_Energies.pdf", savePath.c_str(), tag.Data()));
  delete canvasEta;

  file->Close();
}

void setpad(TVirtualPad *pad, Double_t top = 0.1, Double_t left = 0.15, Double_t right = 0.00, Double_t bottom = 0.15) {
  pad->SetTopMargin(top);
  pad->SetLeftMargin(left);
  pad->SetRightMargin(right);
  pad->SetBottomMargin(bottom);
}

void hset(TGraphErrors &hid, TString xtit = "", TString ytit = "",
          double titoffx = 1.0, double titoffy = 1.0, double titsizex = 0.08,
          double titsizey = 0.1, double labeloffx = 0.01,
          double labeloffy = 0.01, double labelsizex = 0.07,
          double labelsizey = 0.07, int divx = 510, int divy = 510) {
  hid.SetStats(0);

  hid.GetXaxis()->CenterTitle(1);
  hid.GetYaxis()->CenterTitle(1);

  hid.GetXaxis()->SetTitleOffset(titoffx);
  hid.GetYaxis()->SetTitleOffset(titoffy);

  hid.GetXaxis()->SetTitleSize(titsizex);
  hid.GetYaxis()->SetTitleSize(titsizey);

  hid.GetXaxis()->SetLabelOffset(labeloffx);
  hid.GetYaxis()->SetLabelOffset(labeloffy);

  hid.GetXaxis()->SetLabelSize(labelsizex);
  hid.GetYaxis()->SetLabelSize(labelsizey);

  hid.GetXaxis()->SetNdivisions(divx);
  // hid.GetYaxis()->SetNdivisions(divy);
  hid.GetYaxis()->SetNdivisions(5, 5, 0, kTRUE);

  hid.GetXaxis()->SetTitle(xtit);
  hid.GetYaxis()->SetTitle(ytit);
}

int DrawfSam() {

  for (const auto &file : files) {
    fSam(file.first, file.second);
  }
  return 0;
}

TF1 *CreateFitFunc(const TString &tag) {
  TF1 *ff;

  if (tag == "electron") {
    ff = new TF1("exponential_fit",
                 "[0] - [1]/x^[2]"); // Set margin to x-axis than E bins
    ff->SetParameter(0, 0.1);
    ff->SetParameter(1, 5);
    ff->SetParameter(2, 1);
  } else if (tag == "photon") {

    // ff = new TF1("exponential_fit", "[0]/([1] + exp(- [2]*x + [3]))"); // Set
    // margin to x-axis than E bins ff = new TF1("polynomial_fit", "pol9"); ff =
    // new TF1("inverse_exponential_fit", "[0] / ([1] + exp(- [2]*x + [3]) +
    // [4]*x^[5])");
    ff = new TF1("inverse_exponential_fit", "[0] - [1] / x^[2]");
    // ff = new TF1("log_fit", "[0]*log(x+[1])+[2]"); // Set margin to x-axis
    // than E bins ff = new TF1("tanh_fit", "[0]*((exp([1]*x + [2]) -
    // exp(-([1]*x + [2]))) / (exp([1]*x + [2]) + exp(-([1]*x + [2])))) + [3]");
    // // Set margin to x-axis than E bins ff = new TF1("tanh_fit",
    // "[0]*tanh([1]*x + [2]) + [3]"); // Set margin to x-axis than E bins
    ff->SetParameter(0, 0.05);
    ff->SetParameter(1, 0.37);
    ff->SetParameter(2, -0.0013);
    ff->SetParameter(3, -0.064);
    ff->SetParameter(4, -0.73);
    ff->SetParameter(5, 0.028);
  }

  return ff;
}

TString GetFormular(TF1 *tformula) {
  const TString paramSymbols[10] = {
      "[p0]", "[p1]", "[p2]", "[p3]", "[p4]",
      "[p5]", "[p6]", "[p7]", "[p8]", "[p9]",
  };
  TString str = tformula->GetExpFormula("");
  Int_t nparams = tformula->GetNpar();

  // format each parameters (e.g. reduce precision, length of its string, ...)
  for (Int_t i = 0; i < nparams; ++i) {
    Double_t param = tformula->GetParameter(i);
    TString paramString;

    paramString.Form("%.2f", param);
    str.ReplaceAll(paramSymbols[i], paramString);
    std::cout << "formatted param=" << paramString << '\n';
  }
  return str;
}
