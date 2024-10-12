#include <tuple>

#include "TTimeStamp.h"

const std::vector<std::tuple<std::string, std::string, std::string, std::string>> files = {
  {
    "/eic/results/electron_5000evt/",
    "fsam_electron_20240628_91819.root",
    "electron",
  },
  /*
  {
    "/eic/results/photon_5000evt/",
    "fsam_photon_20240617_75733.root",
    "photon"
    ""
  }
  */
};

const std::string edepDir = "/eic/results/electron_2500evt_sensitive";

const std::string savePath = "/eic/results";

const std::vector<double> electron_ebins =
{
    0.5,  0.6,  0.7,  0.8, 
    0.9,  1.0,  1.5,  2.0,
    2.5,  3.0,  4.0,  5.0,
    6.0,  7.0,  8.0,  9.0,
   10.0, 11.0, 12.0, 13.5,
   15.0, 16.5, 18.0
};

const std::vector<double> photon_ebins =
{
    0.1, 0.15, 0.20, 0.25,
    0.3, 0.35, 0.40, 0.45,
    0.5, 0.75,  1.0,  1.5,
    2.0,  2.5,  3.0,  4.0,
    5.0,  6.0,  7.0,  8.0,
    9.0, 10.0, 11.0, 12.0,
   13.5, 15.0, 16.5, 18.0
};



// 128 is arbitrary value for size of static array.
constexpr unsigned int max_ebin = 128;
constexpr unsigned int max_etabin = 128;

static Double_t ETAx[max_etabin];
static Double_t ETAex[max_etabin];
static std::vector<double> ETA_mins;
static std::vector<double> ETA_maxs;
static std::vector<double> ETA_mids;

constexpr std::array ETABINS
{
  -1.7, -1.6, -1.5, -1.0, -0.5,
  0.0,  0.5,  1.0,  1.2,  1.3
};
constexpr Int_t ETAn = ETABINS.size() - 1;

static struct FsamInit {
  FsamInit() {
    for (size_t i = 1; i < ETABINS.size(); ++i) {
      ETAx[i - 1] = (ETABINS[i] + ETABINS[i - 1]) / 2;
      ETAex[i - 1] = (ETABINS[i] - ETABINS[i - 1]) / 2;
    }
    ETA_mins = std::vector<double>(ETABINS.begin(), ETABINS.begin() + ETABINS.size() - 1);
    ETA_maxs = std::vector<double>(ETABINS.begin() + 1, ETABINS.end());
    ETA_mids = std::vector<double>(ETA_mins.size(), 0);
    for (size_t i = 0; i < ETA_mins.size(); ++i) {
      ETA_mids.push_back((ETA_mins[i] + ETA_maxs[i]) / 2);
    }
    TTimeStamp ts;
    date = ts.GetDate();
    time = ts.GetTime();
  }

  ~FsamInit() {
  }

  unsigned int  date;
  unsigned int  time;
} fsamInit;

constexpr Int_t Nevts = 2500;
