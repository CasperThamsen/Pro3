#include <iostream>
#include <stdio.h>
#include <math.h>
#include "portaudio.h"

#include <string>
#include <chrono>
#include <thread>
#include <bitset>
#include <vector>

using namespace std;

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



void asciitobit(char inputChar, bitset<8> &x){
    unsigned char byteValue = static_cast<unsigned char>(inputChar);
    // Display the result as an 8-bit binary string
    bitset<8> binaryRepresentation(byteValue);
    x = binaryRepresentation;
}


void bittotone(string input, char &tone){
    if (input == string("0000")){tone = '1';}
    if (input == string("0001")){tone = '2';}
    if (input == string("0010")){tone = '3';}
    if (input == string("0011")){tone = 'A';}
    if (input == string("0100")){tone = '4';}
    if (input == string("0101")){tone = '5';}
    if (input == string("0110")){tone = '6';}
    if (input == string("0111")){tone = 'B';}
    if (input == string("1000")){tone = '7';}
    if (input == string("1001")){tone = '8';}
    if (input == string("1010")){tone = '9';}
    if (input == string("1011")){tone = 'C';}
    if (input == string("1100")){tone = '*';}
    if (input == string("1101")){tone = '0';}
    if (input == string("1110")){tone = '#';}
    if (input == string("1111")){tone = 'D';}
}


void stringtotone(string import, vector<char> &BitSekvens){
    for (int i = 0; i < import.length(); i++){
        bitset<8> BitSet;
        char input = import[i];
        asciitobit(input, BitSet);
        string test = BitSet.to_string();
        string bit1;
        string bit2;
        for (int j = 0; j < 4; j++){
            bit1 += test[j];
        }
        for (int k = 4; k < 8; k++){
            bit2 += test[k];
        }
        char tone1;
        char tone2;
        bittotone(bit1, tone1);
        bittotone(bit2, tone2);
        BitSekvens.push_back(tone1);
        BitSekvens.push_back(tone2);
    }
}

vector<int> vectorCharToVectorInt(vector<char>& conversionVector) {
    vector<int> integerVector;
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



int main(void){
    PaStreamParameters outputParameters;
    PaStream* stream;
    PaError err;
    paData data;

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



//Input a string to be transmitted.
    std::string stringToBePlayed;
    std::cout << "\nEnter string for transmission: " << std::endl;
    std::cin >> stringToBePlayed;
    std::cout << std::endl;


//Convert from string to a vector of ASCII chars.
    vector<char> asciiChars;    
    stringtotone(stringToBePlayed, asciiChars);
    cout << "ASCII Chars:   ";
    for (int i = 0; i < asciiChars.size(); i++){cout<<asciiChars[i]<<" ";} //Print the ASCII chars for testing/control.
    std::cout << std::endl;


//Convert fron vector of ASCII chars to vector of intergers.
    vector<int> vectorIntToBePlayed = vectorCharToVectorInt(asciiChars);
    cout << "Integer value: ";
    for (int i : vectorIntToBePlayed){std::cout << i << " ";}
    std::cout << std::endl;



    err = Pa_StartStream(stream);
    checkErr(err);

    std::cout << "\nPlaying now:" << std::endl;   

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

    for(int i = 0; i < vectorIntToBePlayed.size(); i++){ //Looper igennem de indtastede chars's toner.
        data.dmtfValg = vectorIntToBePlayed[i];
        std::this_thread::sleep_for(std::chrono::milliseconds(tidAfspillet)); // spiller tone 1 i lang tid.
    }

    // endflag lige pt
    data.dmtfValg = 15;
    std::this_thread::sleep_for(std::chrono::milliseconds(tidAfspillet)); // spiller tone 2

    err = Pa_StopStream(stream);
    checkErr(err);

    err = Pa_CloseStream(stream);
    checkErr(err);

    Pa_Terminate();
    printf("Test finished.\n\n");


    return 0;
}

