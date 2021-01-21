#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include "sample.h"

int main(int carg, char **szarg){
    if(carg != 7){
        std::cout << "Usage:" << std::endl;
        std::cout << "./daq Frequency(Hz) Trigger_level(V) rise/fall Trigger_position(s) Entries File_name_prefix" << std::endl;
        std::cout << "i.e., ./daq 20000000.0 -1.0 rise 0.0 100 test1" << std::endl;
        return 0;
    }
    // Configuration
    const double FREQ = std::stod(szarg[1]); // Hz for PMT
    const double TRIG_LEVEL = std::stod(szarg[2]); // V
    bool TRIG_RISE = false;
    if(szarg[3][0] == 'r'){ TRIG_RISE = true; }
    const double TRIG_POS = std::stod(szarg[4]); // Sec.
    const int ENTRIES = std::stoi(szarg[5]); // Number of Entries in one subrun
    const char* FILE_NAME_PREFIX = szarg[6];
    const double CHANNEL_RANGE = 5.;
    const double CHANNEL_OFFSET = 0.;

    std::ofstream ofs(std::string(FILE_NAME_PREFIX)+".txt", std::ios::app);
    ofs << "File name prefix       : " << FILE_NAME_PREFIX << std::endl;
    ofs << "Sampling Frequency (Hz): " << FREQ << std::endl;
    ofs << "Trigger Level (V)      : " << TRIG_LEVEL << std::endl;
    if(TRIG_RISE){
        ofs << "Trigger Type           : " << "Rising Positive" << std::endl;
    }else{
        ofs << "Trigger Type           : " << "Falling Negative" << std::endl;
    }
    ofs << "Trigger Position (s)   : " << TRIG_POS << std::endl;
    ofs << "Range (V)              : " << CHANNEL_RANGE << std::endl;
    ofs << "Offset (V)             : " << CHANNEL_OFFSET << std::endl;
    ofs << "Number of Events       : " << ENTRIES << std::endl;

    HDWF hdwf;
    STS sts;
    int cSamples = 8192;
    short *rgdSamples = new short[cSamples];
    short *previousSamples = new short[cSamples];

    for(int i=0;i<cSamples;++i){
        rgdSamples[i] = 0;
        previousSamples[i] = 0;
    }
    char szError[512] = {0};
    
    printf("Open automatically the first available device\n");
    if(!FDwfDeviceOpen(-1, &hdwf)) {
        FDwfGetLastErrorMsg(szError);
        printf("Device open failed\n\t%s", szError);
        ofs << "-1 Error: Device open failed." << std::endl;
        return 0;
    }

    FDwfAnalogInFrequencySet(hdwf, FREQ);
    FDwfAnalogInBufferSizeSet(hdwf, 8192);
    FDwfAnalogInChannelEnableSet(hdwf, 0, true);
    FDwfAnalogInChannelRangeSet(hdwf, 0, CHANNEL_RANGE);
    FDwfAnalogInChannelOffsetSet(hdwf, 0, CHANNEL_OFFSET);

    

    // set up trigger
    // disable auto trigger
    FDwfAnalogInTriggerAutoTimeoutSet(hdwf, 0);
    // one of the analog in channels
    FDwfAnalogInTriggerSourceSet(hdwf, trigsrcDetectorAnalogIn);
    FDwfAnalogInTriggerTypeSet(hdwf, trigtypeEdge);
    // first channel
    FDwfAnalogInTriggerChannelSet(hdwf, 0);
    FDwfAnalogInTriggerLevelSet(hdwf, TRIG_LEVEL); // Volt
    if(TRIG_RISE){
        FDwfAnalogInTriggerConditionSet(hdwf, trigcondRisingPositive);
    }else{
        FDwfAnalogInTriggerConditionSet(hdwf, trigcondFallingNegative);
    }
    FDwfAnalogInTriggerPositionSet(hdwf, TRIG_POS);

    Wait(2); // have to

    FDwfAnalogInConfigure(hdwf, false, true);

    const int nevents_in_file = 1000;
    FILE *outputfile;

    struct timeval _time_now;
    gettimeofday(&_time_now, NULL);
    struct timeval _time;
    gettimeofday(&_time, NULL);
    struct timeval _time_release;
    long time_to_calc_rate = _time_now.tv_sec;
    printf("Starting repeated acquisitions.\n");



    for(int iTrigger = 0; iTrigger < ENTRIES; iTrigger++){
        if(iTrigger % 100 == 0){
            printf("Ev. %d\n", iTrigger);
        }
        // File close and open
        if(iTrigger % nevents_in_file == 0){
            if(iTrigger != 0) {fclose(outputfile);}
            std::stringstream ss;
            ss << FILE_NAME_PREFIX << "." << std::setw(4) << std::setfill('0') << int(iTrigger/nevents_in_file) << ".dat";
            //printf("file %s\n", ss.str().c_str());
            outputfile = fopen(ss.str().c_str(), "w");
            if (outputfile == NULL) {
                printf("cannot open file.\n");
                ofs << "-2 Error: Cannot open file." << std::endl;
                return 0;
            }
        }


        while(true){
            FDwfAnalogInStatus(hdwf, true, &sts);
            if(sts == DwfStateDone) {
                break;
            }
            gettimeofday(&_time_now, NULL);
            if(_time_now.tv_sec - _time.tv_sec > 600){
                printf("Error: No events in this 600 seconds. Quit.");
                ofs << "-3 Error: No events in this 600 seconds. Quit." << std::endl;
                ofs.close();
                fclose(outputfile);
                FDwfDeviceCloseAll();
                return 0;
            }
            Wait(0.001);
        }
		gettimeofday(&_time, NULL);
		long sec = _time.tv_sec;
		long usec = _time.tv_usec;
        fprintf(outputfile, "#. %d %ld %ld\n", iTrigger, sec, usec);

        FDwfAnalogInStatusData16(hdwf, 0, rgdSamples, 0, cSamples);

        // Error Check
        bool isSamePrevious = true;

        for(int i = 0; i < cSamples; i++){
            if(rgdSamples[i] != previousSamples[i]){
                isSamePrevious = false;
                break;
            }
        }
        if(isSamePrevious){
            printf("Error: New event is the same of the previous one.");
            ofs << "-4 Error: New event is the same of the previous one." << std::endl;
            ofs.close();
            fclose(outputfile);
            FDwfDeviceCloseAll();
            return 0; 
        }

        for(int i = 0; i < cSamples; i++){
            fprintf(outputfile,"%d\n", rgdSamples[i]);
            previousSamples[i] = rgdSamples[i];
        }

        // Calc rate
        if(iTrigger % 10 == 0 && iTrigger != 0){
            double diff = double(sec - time_to_calc_rate);
            if (diff < 0.1) diff = 10./9999.;
            ofs << "Rate(Hz) " << iTrigger << " " << sec << " " << 10./diff << std::endl;
            time_to_calc_rate = sec;
        }

		gettimeofday(&_time_release, NULL);
		long sec_release = _time_release.tv_sec;
		long usec_release = _time_release.tv_usec;
        fprintf(outputfile, "%ld %ld\n", sec_release, usec_release);
    }

    // terminate
    ofs << "0 Finished successfully." << std::endl;
    ofs.close();
    fclose(outputfile);
    FDwfDeviceCloseAll();

    return 0;
}
