#include <iostream>
#include <stdio.h>
#include <math.h>
#include "portaudio.h"
#include <vector>
#include <fstream>
#include <unistd.h>
#include <string>

#define SAMPLE_RATE (44100)
#define TABLE_SIZE (200)


typedef struct{
    float dtmf[17][TABLE_SIZE];
    unsigned int dmtfValg;
    unsigned int phase;
} paData;






//HER
std::vector<int> sequenceCharToSequenceInt(const std::vector<char>& conversionVector) {
    std::vector<int> integerVector;
    for (char c : conversionVector) {
        switch (c) {
            case '1': integerVector.push_back(0); break;
            case '2': integerVector.push_back(1); break;
            case '3': integerVector.push_back(2); break;
            case 'A': integerVector.push_back(3); break;
            case '4': integerVector.push_back(4); break;
            case '5': integerVector.push_back(5); break;
            case '6': integerVector.push_back(6); break;
            case 'B': integerVector.push_back(7); break;
            case '7': integerVector.push_back(8); break;
            case '8': integerVector.push_back(9); break;
            case '9': integerVector.push_back(10); break;
            case 'C': integerVector.push_back(11); break;
            case '*': integerVector.push_back(12); break;
            case '0': integerVector.push_back(13); break;
            case '#': integerVector.push_back(14); break;
            case 'D': integerVector.push_back(15); break;
            default: break;
        }
    }
    return integerVector;
}
//HER







static int audioCallback(const void* inputBuffer, void* outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData)
{
    paData* data = (paData*)userData;
    float* out = (float*)outputBuffer;

    (void)timeInfo;
    (void)statusFlags;
    (void)inputBuffer;


    //Sammensætter og gør lyden klar til afspildning via *out++
    static unsigned long i;
    for (i = 0; i < framesPerBuffer; i++){
        *out++ = data->dtmf[data->dmtfValg][data->phase];
        if(data->dmtfValg == 16){
            *out++ = 0.0f;
        }
        data->phase +=  1;
        if (data->phase >= TABLE_SIZE) data->phase -= TABLE_SIZE;
    }

    return paContinue;
}

int main(void)
{
    PaStreamParameters outputParameters;
    PaStream* stream;
    paData data;

    // DTMF frekvenser
    float freqRow[4] = {1209,1336,1477,1633};
    float freqcol[4] = {679,770,852,941};
    const float amplitude = 0.5;

    // Udregner DTMF tonerne vha. for loop, og gemmer dem i data.dtmf
    //Udregner dem som 1..2..3..A..4 Osv...
    float sin1, sin2 = 0;
    int j =0;
    for(int l = 0; l < 4; l++){
        for(int k = 0; k < 4; k++){
            for (int i = 0; i < TABLE_SIZE; i++){
                sin1 = amplitude * sin(2 * M_PI * freqRow[k] * i / SAMPLE_RATE);
                sin2 = amplitude * sin(2 * M_PI * freqcol[l] * i / SAMPLE_RATE);
                data.dtmf[j][i] = sin1 + sin2;
            }
            j++;
        }
    }
    
    data.phase = 0;

//TEST
    std::vector<char> sequenceChar{'1', '2', '3', 'A', '4', '5', '6', 'B', '7', '8', '9', 'C', '*', '0', '#', 'D'};
    std::vector<int> sequenceInteger = sequenceCharToSequenceInt(sequenceChar);

    for (int i : sequenceInteger) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
//TEST

   Pa_Initialize();
    

    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice){
        fprintf(stderr, "Error: No default output device.\n");
        exit(EXIT_FAILURE);
    }
    
    outputParameters.channelCount = 1;        // Mono output
    outputParameters.sampleFormat = paFloat32; // 32-bit floating-point output
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

        Pa_OpenStream(
        &stream,
        NULL,                 // No input
        &outputParameters,
        SAMPLE_RATE,
        64,
        paClipOff,             // Don't clip out-of-range samples
        audioCallback,
        &data);
   

    Pa_StartStream(stream);
       
    for(int i = 0; i < sequenceInteger.size(); i++){ // looper igennem sekvensen
        data.dmtfValg = sequenceInteger[i];
        Pa_Sleep(200);
    } 
    

    Pa_StopStream(stream);
    

    Pa_CloseStream(stream);
    

    Pa_Terminate();
    return 0;
}