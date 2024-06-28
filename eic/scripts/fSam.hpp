// 128 is arbitrary value for size of static array.
constexpr unsigned int max_ebin = 128;
constexpr unsigned int max_etabin = 128;

const std::vector<std::pair<TString, TString>> files = {
    {"/usr/local/share/eic/results/electron_5000evt/rec/results/"
     "fsam_electron_20240617_75559.root",
     "electron"},
    // TODO: use updated root
    {"/usr/local/share/eic/results/photon_5000evt/rec/results/"
     "fsam_photon_20240617_75733.root",
     "photon"}};

const std::string savePath = "/eic/results";

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

constexpr std::array ETABINS
{
  -1.7, -1.6, -1.5, -1.0, -0.5,
  0.0,  0.5,  1.0,  1.2,  1.3
};

Int_t Nevts = 5000;
