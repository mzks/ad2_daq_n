// ----------------------------- //
// conv.cpp C++ script
// Keita Mizukoshii
// Usage
// ./conv run_name file_name
// i.e.,
// ./conv 20200604 sub0136.0000
// ----------------------------- //

#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"

int main(int argc, char** argv){

	if(argc != 3){
		std::cout << "Usage :" << std::endl;
		std::cout << "./conv run_name file_name" << std::endl;
		return -1;
	}

	TString run_name = argv[1];
	TString filename = argv[2];

	TString data_dir = "/home/mzks/ad2_daq/data/";
	TString root_dir = "/home/mzks/ad2_daq/rootfile/";
	std::cout << data_dir+run_name+"/"+filename+".dat" << std::endl;

	std::ifstream ifs(data_dir+run_name+"/"+filename+".dat");
	if(!ifs.is_open()){
		std::cout << "no such file" << std::endl;
		return -1;
	}

	std::string buf;
	std::string buf_ts;
	ULong64_t eventid;
	ULong64_t timestamp;
	UInt_t timestamp_usec;
	ULong64_t timestamp_end;
	UInt_t timestamp_usec_end;
	Short_t buf1;
	Float_t wf[8192];

	mkdir((root_dir+run_name).Data(), 0777);

	TFile* out_file = TFile::Open(root_dir+run_name+"/"+filename+".root", "RECREATE");

	TTree* tree = new TTree("tree", "tree");
	tree->Branch("eventid", &eventid, "eventid/l");
	tree->Branch("timestamp", &timestamp, "timestamp/l");
	tree->Branch("timestamp_usec", &timestamp_usec, "timestamp_usec/i");
	tree->Branch("timestamp_end", &timestamp_end, "timestamp_end/l");
	tree->Branch("timestamp_usec_end", &timestamp_usec_end, "timestamp_usec_end/i");
	tree->Branch("wf", wf, "wf[8192]/F");

	std::string token;

	while(ifs >> buf >> eventid >> timestamp >> timestamp_usec){

		for(int ibin=0;ibin<8192;++ibin){
			ifs >> buf1;
			wf[ibin] = buf1*5./65536.;
		}

		ifs >> timestamp_end >> timestamp_usec_end;

		tree->Fill();
	}
	tree->Write();
	out_file->Close();

	return 0;
}
