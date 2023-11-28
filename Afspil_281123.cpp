#include <iostream>
#include <stdio.h>
#include <math.h>
#include "portaudio.h"

#include <fstream>
#include <string>
#include <chrono>
#include <thread>


#define SAMPLE_RATE (8000)
#define NUM_SECONDS (2)
#define TABLE_SIZE (8000)

typedef struct{
    float dtmf[16][TABLE_SIZE];  //16
    unsigned int dmtfValg;
    unsigned int phase;
} paData;


static void checkErr(PaError err){
    if(err != paNoError){
        std::cout << "PortAudio error " << Pa_GetErrorText(err) << std::endl;
        exit(EXIT_FAILURE);
    }
}

static int audioCallback(const void* inputBuffer, void* outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData){
    paData* data = (paData*)userData;
    float* out = (float*)outputBuffer;

    (void)timeInfo;
    (void)statusFlags;
    (void)inputBuffer;

    static unsigned long i;
    for (i = 0; i < framesPerBuffer; i++){

        *out++ = data->dtmf[data->dmtfValg][data->phase];

        data->phase +=  1;
        if( data->phase >= TABLE_SIZE ) data->phase -= TABLE_SIZE; //fjerner skrætten
    }

    return paContinue;
}



int main(void){
    PaStreamParameters outputParameters;
    PaStream* stream;
    PaError err;
    paData data;

    std::cout << "Playing" << std::endl;

    float freqRow[4] = {1209,1336,1477,1633};
    float freqCol[4] = {679,770,852,941};
    const float amplitude = 0.5;

    float sin1, sin2 = 0;
    int j =0;
    for(int l = 0; l < 4; l++){
        for(int k = 0; k < 4; k++){
            for (int i = 0; i < TABLE_SIZE; i++){
                sin1 = amplitude * sin(2 * M_PI * freqRow[k] * i / SAMPLE_RATE);
                sin2 = amplitude * sin(2 * M_PI * freqCol[l] * i / SAMPLE_RATE);
                data.dtmf[j][i] = sin1 + sin2;
            }
            j++;
        }
    }

    data.phase = 0;

    err = Pa_Initialize();
    checkErr(err);

    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice){
        fprintf(stderr, "Error: No default output device.\n");
        exit(EXIT_FAILURE);
    }
    
    outputParameters.channelCount = 1;        // Mono output
    outputParameters.sampleFormat = paFloat32; // 32-bit floating-point output
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream,
        NULL,                 // No input
        &outputParameters,
        SAMPLE_RATE,
        paFramesPerBufferUnspecified,
        paClipOff,             // Don't clip out-of-range samples
        audioCallback,
        &data);
    checkErr(err);

    err = Pa_StartStream(stream);
    checkErr(err);

    printf("Playing for %d seconds.\n", NUM_SECONDS);
    

    int tidAfspillet = (21*3); // buffertid er 21

    // Start sekvens:
    // 1 5 9 D
    data.dmtfValg = 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(tidAfspillet)); // spiller tone 1 i lang tid.
    

    data.dmtfValg = 5;
    std::this_thread::sleep_for(std::chrono::milliseconds(tidAfspillet)); // spiller tone 2

    data.dmtfValg = 10;
    std::this_thread::sleep_for(std::chrono::milliseconds(tidAfspillet)); // spiller tone 3


    data.dmtfValg = 15;
    std::this_thread::sleep_for(std::chrono::milliseconds(tidAfspillet)); // spiller tone 3

    // auto startTimeTest = std::chrono::high_resolution_clock::now();
    // auto CurrentTimeTest = std::chrono::high_resolution_clock::now();
    // auto elapsedTimeTest = std::chrono::duration_cast<std::chrono::milliseconds>(CurrentTimeTest - startTimeTest).count();

    int antalAfspillet = 0;
    char bgst[16] = {'1','2','3','A','4','5','6','B','7','8','9','C','*','0','#','D'};
    // for(int j = 0; j < 4; j++){
        for(int i = 0; i < 15; i++){ // looper igennem de 16 lyde

            // CurrentTimeTest = std::chrono::high_resolution_clock::now();
            // elapsedTimeTest = std::chrono::duration_cast<std::chrono::milliseconds>(CurrentTimeTest - startTimeTest).count();
            // std::cout << "StartTest tid siden sidst : " << elapsedTimeTest << " milisekunder." <<std::endl;

            // std::cout << "Tone " << bgst[i] << " Tid: "<< antalAfspillet << std::endl;

            data.dmtfValg = i;
            std::this_thread::sleep_for(std::chrono::milliseconds(tidAfspillet)); // spiller tone 1 i lang tid.
            // std::cout << "" << std::endl;
            // antalAfspillet++;  
        } 
    // }

    //endflag lige pt
    data.dmtfValg = 15;
    std::this_thread::sleep_for(std::chrono::milliseconds(tidAfspillet)); // spiller tone 2

    err = Pa_StopStream(stream);
    checkErr(err);

    err = Pa_CloseStream(stream);
    checkErr(err);

    Pa_Terminate();
    printf("Test finished.\n");


    return 0;
}
