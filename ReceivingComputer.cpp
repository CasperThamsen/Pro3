#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include "portaudio.h"
#include "fftw3.h"
#include <bits/stdc++.h>
#include <complex>
#include <chrono>
#include <thread>
#include <queue>
#include <signal.h>

#define SAMPLE_RATE_RECORD (44100.0)
#define FRAMES_PER_BUFFER_RECORD (900) 

using namespace std;


/////////////////////////////////////RECORD//////////////////////////////////////////////////////
typedef struct {
    double* in;      // Input buffer, will contain our audio sample
    double* out;
    fftw_plan plan;
    int finished = paContinue;
} paRecordData;

static paRecordData* Data; //opretter den globalt

vector<char> freqVals;

queue<double> callbackInfo; //kan tænkes som en vektor buffer der kan fjernes for fra
bool TimingBool = false; //checker i FFT
auto startTime = std::chrono::high_resolution_clock::now();


int timeCount = 0;
const int bufferTime = 40;
int recordingTime = (bufferTime*6) - 2; //Bufferen skal være 240

vector<char> tonesCurrentTime;
vector<char> everyToneVector; //everyToneVector
char toneCharfound;
/////////////////////////////////////RECORD SLUT//////////////////////////////////////////////////////


static void checkErr(PaError err) {
    if (err != paNoError) {
        std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        exit(EXIT_FAILURE);
    }
}


bool checkPattern(string str, string pattern) {
    //https://www.geeksforgeeks.org/check-string-follows-order-characters-defined-pattern-not/
        // len stores length of the given pattern 
    int len = pattern.length();

    for (int i = 0; i < len - 1; i++) {
        // x, y are two adjacent characters in pattern
        char x = pattern[i];
        char y = pattern[i + 1];

        // find index of last occurrence of character x
        // in the input string
        size_t last = str.find_last_of(x);

        // find index of first occurrence of character y
        // in the input string
        size_t first = str.find_first_of(y);

        // return false if x or y are not present in the 
        // input string OR last occurrence of x is after 
        // the first occurrence of y in the input string
        if (last == string::npos || first == string::npos || last > first) {
            return false;
        }
    }
    return true;
}

char getMaxOccurringChar(vector<char>& CharVec) {
    //https://www.geeksforgeeks.org/return-maximum-occurring-character-in-the-input-string/
    std::string toneStr(CharVec.begin(), CharVec.end());
    // create unordered_map to store frequency of every character
    unordered_map<char, int>mp;
    char ans;
    int cnt = 0;

    for (int i = 0;i < toneStr.length(); i++) {
        // push element into map and increase its frequency 
        mp[toneStr[i]]++;

        // update answer and count
        if (cnt < mp[toneStr[i]]) {
            ans = toneStr[i];
            cnt = mp[toneStr[i]];
        }
    }
    return ans;
}

void checkForStartFlag(vector<char> CharVector) {
    const std::string startFlag = "159D";
    const int lookBack = 30; //antal af karaketere vi kigger tilbage
    if ((CharVector.size() >= lookBack)) {
        std::string lastTones(CharVector.end() - lookBack, CharVector.end());
        if (checkPattern(lastTones, startFlag)) {
            std::cout << "Start flag detected!" << std::endl;
            TimingBool = true;
            startTime = std::chrono::high_resolution_clock::now();
            CharVector.clear();
        }
    }
}

void checkForStopFlag(vector<char> CharVector) {
    const std::string stopflag = "2*BD";
    const int lookBack = 4;
    if ((CharVector.size() >= lookBack)) {
        std::string lastTones(CharVector.end() - lookBack, CharVector.end());
        if (checkPattern(lastTones, stopflag)) {
            std::cout << "stop flag detected!" << std::endl;
            Data->finished = paComplete;
        }
    }
}


void recognizeTone(double upperFreq, double lowerFreq, char& toneCharfound) {
    double at = 25;//33 //Tilladt afvigelse
    double atstor = 25; //63
    std::vector<double> DTMFfreq{ 1209, 1336, 1477, 1633, 679, 770, 852, 941 };

    if (DTMFfreq[0] - atstor < upperFreq && DTMFfreq[0] + atstor > upperFreq) {
        if (DTMFfreq[4] - at < lowerFreq && DTMFfreq[4] + at > lowerFreq) { toneCharfound = '1'; }
        if (DTMFfreq[5] - at < lowerFreq && DTMFfreq[5] + at > lowerFreq) { toneCharfound = '4'; }
        if (DTMFfreq[6] - at < lowerFreq && DTMFfreq[6] + at > lowerFreq) { toneCharfound = '7'; }
        if (DTMFfreq[7] - at < lowerFreq && DTMFfreq[7] + at > lowerFreq) { toneCharfound = '*'; }
    }
    else if (DTMFfreq[1] - atstor < upperFreq && DTMFfreq[1] + atstor > upperFreq) {
        if (DTMFfreq[4] - at < lowerFreq && DTMFfreq[4] + at > lowerFreq) { toneCharfound = '2'; }
        if (DTMFfreq[5] - at < lowerFreq && DTMFfreq[5] + at > lowerFreq) { toneCharfound = '5'; }
        if (DTMFfreq[6] - at < lowerFreq && DTMFfreq[6] + at > lowerFreq) { toneCharfound = '8'; }
        if (DTMFfreq[7] - at < lowerFreq && DTMFfreq[7] + at > lowerFreq) { toneCharfound = '0'; }
    }
    else if (DTMFfreq[2] - atstor < upperFreq && DTMFfreq[2] + atstor > upperFreq) {
        if (DTMFfreq[4] - at < lowerFreq && DTMFfreq[4] + at > lowerFreq) { toneCharfound = '3'; }
        if (DTMFfreq[5] - at < lowerFreq && DTMFfreq[5] + at > lowerFreq) { toneCharfound = '6'; }
        if (DTMFfreq[6] - at < lowerFreq && DTMFfreq[6] + at > lowerFreq) { toneCharfound = '9'; }
        if (DTMFfreq[7] - at < lowerFreq && DTMFfreq[7] + at > lowerFreq) { toneCharfound = '#'; }
    }
    else if (DTMFfreq[3] - atstor < upperFreq && DTMFfreq[3] + atstor > upperFreq) {
        if (DTMFfreq[4] - at < lowerFreq && DTMFfreq[4] + at > lowerFreq) { toneCharfound = 'A'; }
        if (DTMFfreq[5] - at < lowerFreq && DTMFfreq[5] + at > lowerFreq) { toneCharfound = 'B'; }
        if (DTMFfreq[6] - at < lowerFreq && DTMFfreq[6] + at > lowerFreq) { toneCharfound = 'C'; }
        if (DTMFfreq[7] - at < lowerFreq && DTMFfreq[7] + at > lowerFreq) { toneCharfound = 'D'; }
    }
    else {
        toneCharfound = 'm'; // m for mellemrum i vektoren.
    }

    //fjerner et underligt NULL
    if ((int)toneCharfound == 0) {
        cout << "Null Fundet" << endl;
        return;
    }
}

void computingFFT(char& toneCharfound) {
    fftw_execute(Data->plan); //gør fft'en. Nu er out fyldt med data som er transformet af fft'en

    int firBigIndex = -1;
    int secBigIndex = -1;

    for (int i = 0; i < FRAMES_PER_BUFFER_RECORD/2; i++) { //Kun igennem halvdelen (/2)
        if (Data->out[i] * Data->out[i] > Data->out[firBigIndex] * Data->out[firBigIndex]) { //magnitude, hvor kvadratroden er "sparet" væk
            if (((double(i) * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD / 2) > 1100) && ((double(i) * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD / 2) < 1700)) {
                firBigIndex = i;
            }
        }
        if (Data->out[i] * Data->out[i] > Data->out[secBigIndex] * Data->out[secBigIndex]) { //magnitude, hvor kvadratroden er "sparet" væk
            if (((double(i) * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD / 2) > 600) && ((double(i) * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD / 2) < 1000)) {
                secBigIndex = i;
            }
        }
    }

    double bigFoundFreq = (double)firBigIndex * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD / 2; //Vi dividere med 2 pga. spejlning af resultatet.
    double smallFoundFreq = (double)secBigIndex * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD / 2;

    //Har igen effekt da if-statementsene i forloopet ovenfor allerede har styr på, at de rigtige amplituder.
    if (smallFoundFreq > bigFoundFreq) {
        double temp = bigFoundFreq;
        bigFoundFreq = smallFoundFreq;
        smallFoundFreq = temp;
    }
    recognizeTone(bigFoundFreq, smallFoundFreq, toneCharfound);
}

void FFT() {
    while ((callbackInfo.size() >= (FRAMES_PER_BUFFER_RECORD)) && (!callbackInfo.empty())) {

        for (int i = 0; i < FRAMES_PER_BUFFER_RECORD; i++) {
            Data->in[i] = callbackInfo.front(); //tager index 0.
            callbackInfo.pop(); //sletter idex 0, index 1 bliver ny index 0
        }

        computingFFT(toneCharfound);
        std::cout << toneCharfound << std::endl;

        freqVals.push_back(toneCharfound);

        if (TimingBool && toneCharfound != 'm') {
            tonesCurrentTime.push_back(toneCharfound);
        }

        if (!TimingBool) {
            checkForStartFlag(freqVals);
        }
        else {
            checkForStopFlag(everyToneVector);
        }

        auto CurrentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(CurrentTime - startTime).count();
        if (((elapsedTime > (recordingTime * 1000)) && TimingBool)) { // *1000 fordi tjek er i micro sekunder.
            startTime = std::chrono::high_resolution_clock::now();
            timeCount++;
            std::cout << "                      siden sidst " << elapsedTime << std::endl;
            std::cout << "                      Ny tid " << timeCount << std::endl;

            if ((timeCount > 0) && (tonesCurrentTime.size() > 0)) {
                everyToneVector.push_back(getMaxOccurringChar(tonesCurrentTime));
                tonesCurrentTime.clear();
            }
            else {
                tonesCurrentTime.clear();
            }
        }
    }
}

static int recordCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {

    paRecordData* data = (paRecordData*)userData;

    (void)outputBuffer;//Disse skal ikke retunere noget. Vi bruger dem ikke. 
    (void)timeInfo;
    (void)statusFlags;

    //En pointer til addressen for inputbufferen (den optagne lyd) i formatet en double.
    double* in = (double*)inputBuffer;

    for (unsigned long i = 0; i < FRAMES_PER_BUFFER_RECORD; i++) {
        callbackInfo.push(in[i]); //overføre til vores "ringe buffer"
    }
    FFT();

    if (data->finished == paComplete) {
        return paComplete;
    }

    //Retunerer finished, som enten retunere "paContinue" (forsæt optagelse) eller "paComplete" (færdig med optagelse).
    return paContinue;
}


void toneToBit(vector<char>& Tones, vector<string>& BitSets) {
    string Placeholder;
    for (int i = 0; i < Tones.size(); i++) {
        if (Tones[i] == '1') { Placeholder = ("0000"); }
        if (Tones[i] == '2') { Placeholder = ("0001"); }
        if (Tones[i] == '3') { Placeholder = ("0010"); }
        if (Tones[i] == 'A') { Placeholder = ("0011"); }
        if (Tones[i] == '4') { Placeholder = ("0100"); }
        if (Tones[i] == '5') { Placeholder = ("0101"); }
        if (Tones[i] == '6') { Placeholder = ("0110"); }
        if (Tones[i] == 'B') { Placeholder = ("0111"); }
        if (Tones[i] == '7') { Placeholder = ("1000"); }
        if (Tones[i] == '8') { Placeholder = ("1001"); }
        if (Tones[i] == '9') { Placeholder = ("1010"); }
        if (Tones[i] == 'C') { Placeholder = ("1011"); }
        if (Tones[i] == '*') { Placeholder = ("1100"); }
        if (Tones[i] == '0') { Placeholder = ("1101"); }
        if (Tones[i] == '#') { Placeholder = ("1110"); }
        if (Tones[i] == 'D') { Placeholder = ("1111"); }
        BitSets.push_back(Placeholder);
    }
}

void bitToByte(vector<string>& BitsetstoByte) {
    vector<string> tempvec;
    for (int i = 0; i < BitsetstoByte.size(); i += 2) { //takes every bitset and combines them to a byte
        string string1 = BitsetstoByte[i] + BitsetstoByte[i + 1];
        tempvec.push_back(string1);
    }
    BitsetstoByte = tempvec;
}

void byteToAscii(string& asciiString, vector<string>& byte) {
    string Placeholder;
    for (int i = 0; i < byte.size(); i++) {
        int asciiVal = bitset<8>(byte[i]).to_ulong(); //takes the byte and converts it into a ascii char
        Placeholder.push_back(static_cast<char>(asciiVal));
    }
    asciiString = Placeholder;
}

void toneToAscii(vector<char>& input, string& output) {
    vector<string> placeholder;
    toneToBit(input, placeholder);
    bitToByte(placeholder);
    byteToAscii(output, placeholder);
}


int main(int argc, char** argv) {
    PaStreamParameters inputParameters;
    PaStream* stream;
    PaError err;

    err = Pa_Initialize();
    checkErr(err);

    Data = (paRecordData*)fftw_malloc(sizeof(paRecordData));
    Data->in = (double*)fftw_malloc(sizeof(double) * FRAMES_PER_BUFFER_RECORD);
    Data->out = (double*)fftw_malloc(sizeof(double) * FRAMES_PER_BUFFER_RECORD);
    Data->plan = fftw_plan_r2r_1d(FRAMES_PER_BUFFER_RECORD, Data->in, Data->out, FFTW_R2HC, FFTW_ESTIMATE);

    inputParameters.device = Pa_GetDefaultInputDevice(); //default input device 

    inputParameters.channelCount = 1; // Mono input 
    inputParameters.sampleFormat = paFloat32; // Sample format er float
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        NULL, // &outputParameters, 
        SAMPLE_RATE_RECORD,
        FRAMES_PER_BUFFER_RECORD, //paFramesPerBufferUnspecified
        paNoFlag,
        recordCallback,
        Data);    
    checkErr(err);

    err = Pa_StartStream(stream);
    checkErr(err);

    std::cout << "\n=== Now recording!! ===\n" << std::endl;

    while ((err = Pa_IsStreamActive(stream)) == 1) { //Optager intil array er udfyldt
        Pa_Sleep(10);
    }

    err = Pa_StopStream(stream);
    checkErr(err);

    err = Pa_CloseStream(stream);
    checkErr(err);

    std::cout << "\n=== Done recording!! ===" << std::endl << std::endl;

    fftw_destroy_plan(Data->plan);
    fftw_free(Data->in);
    fftw_free(Data->out);
    fftw_free(Data);
    fftw_cleanup();

    //pop den første
    everyToneVector.erase(everyToneVector.begin());

    //pop de 3 sidste
    everyToneVector.pop_back();
    everyToneVector.pop_back();
    everyToneVector.pop_back();
    everyToneVector.pop_back();

    // printer til terminalen for error check i mens
    cout << "Antal værdier: " << everyToneVector.size() << endl;
    cout << "Toner:" << endl;
    for (int i = 0; i < everyToneVector.size(); i++) {
        cout << everyToneVector[i] << ",";
        if (0 == ((i + 1) % 2)) { cout << " "; }
    }
    cout << endl;

    string output;
    toneToAscii(everyToneVector, output);
    cout << "Anden computer siger: " << output << endl;

    Pa_Terminate();
    checkErr(err);

    return 0;
}
