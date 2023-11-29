#include <iostream>
#include <string>

#include "portaudio.h"
#include "fftw3.h"

#include <bits/stdc++.h>

#include <complex>
#include <chrono>
#include <thread>
#include <queue>


#define SAMPLE_RATE (44100.0)
#define FRAMES_PER_BUFFER (900) //Normalt 1024 eller 512

using namespace std;

/*Variable*/
typedef struct{
    double* in;      // Input buffer, will contain our audio sample
    double* out;
    fftw_plan plan;
    int finished = paContinue;
} paTestData;

static paTestData* Data; //opretter den globalt


vector<char> freqVals; /*
vector fucker lidt med alloceringen 
når den/vi ikke kender den endelige størrelse på forhånd
så lavede det til en alm global variabel i stedet.
Kan godt oprettes i main
*/

bool FFTThreadBoolStop = true;
queue<double> callbackInfo; //"ringbuffer" agtig kan tænkes som en vektor buffer der kan fjernes for fra

int SyncBool = 0; // checker i FFT
bool TimingBool = false; //checker i FFT
bool NyTid = false; // til når den ikke kan genkende nogle lyde, så er der formeltligt 2 lyde i 1 buffer.
int buffantalTest = 0; // callback antal buffer siden start flag
auto nuTid = std::chrono::high_resolution_clock::now(); //til callbackens timer
auto startTime = std::chrono::high_resolution_clock::now();



static void checkErr(PaError err){
    if(err != paNoError){
        std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void removeRepeatedTones(vector<char> & Charvector){   
    for (int i = 0; i < Charvector.size()-1; i++){
        if(Charvector[i] == Charvector[i+1]){
            Charvector.erase(Charvector.begin()+i);
            --i;
        }
    }
    // for (int i = 0; i < Charvector.size()-1; i++){
    //     if((Charvector[i] == 'm') || ((int)Charvector[i] == 0)){
    //         Charvector.erase(Charvector.begin()+i);
    //         --i;
    //     }
    // }
}


bool checkPattern(string str, string pattern){
//https://www.geeksforgeeks.org/check-string-follows-order-characters-defined-pattern-not/
    // len stores length of the given pattern 
    int len = pattern.length();
      
    // if length of pattern is more than length of 
    // input string, return false;
    if (str.length() < len)
        return false;
      
    for (int i = 0; i < len - 1; i++) 
    {
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
        if (last == string::npos || first ==  
            string::npos || last > first)    
        return false;
    }
      
    // return true if string matches the pattern
    return true;
}

char getMaxOccurringChar(vector<char> & nyTidCharVector){   
    //https://www.geeksforgeeks.org/return-maximum-occurring-character-in-the-input-string/

    std::string nytidtone(nyTidCharVector.begin(), nyTidCharVector.end());

    // create unordered_map to store frequency of every character
    unordered_map<char,int>mp; 
    char ans;
    int cnt=0;
     
    for(int i=0 ;i<nytidtone.length() ; i++){
        // push element into map and increase its frequency 
        mp[nytidtone[i]]++;
         
        // update answer and count
        if(cnt < mp[nytidtone[i]]){
            ans = nytidtone[i];
            cnt = mp[nytidtone[i]];
        }
    }
     
    return ans;     
}


void checkForFlag(vector<char> & Charvector) {
    const std::string startFlag = "159D";
    const std::string stopflag = "2*BD"; // skal laves om til en vi er sikker på ikke kan komme i ACII karaktere.
    int sidsteTiNumer = 30; //4*3buffer = 12 så mindst se 10 tilbage

    if ((Charvector.size() >= sidsteTiNumer) && (!TimingBool)) {
        std::string lastTones(Charvector.end() - sidsteTiNumer, Charvector.end());

        if (checkPattern(lastTones, startFlag)) {
            std::cout << "Start flag detected!" << std::endl;
            std::cout << "" << std::endl;
            std::cout << "" << std::endl;

            TimingBool = true;

            nuTid = std::chrono::high_resolution_clock::now(); //FFT tid sørge for at de små spikes ikke kommer med.            
            startTime = std::chrono::high_resolution_clock::now();

            std::cout << "Start flag observeret" << std::endl;
            std::cout << "Empty" << std::endl;
            Charvector.clear();
        }
    }

    if((Charvector.size() >= sidsteTiNumer) && (TimingBool)){
        std::string lastTones(Charvector.end() - sidsteTiNumer, Charvector.end());

        if (checkPattern(lastTones, stopflag)) {
            std::cout << "stop flag detected!" << std::endl;
            Data->finished = paComplete;
        }
    }
}


void genkendDTMFtoner(double stor, double lille, char& genkendtTone){ 
    double at = 25;//35 //Tilladt afvigelse
    double atstor = 25; //63
    std::vector<double> freq{1209, 1336, 1477, 1633, 679, 770, 852, 941};

    if(freq[0]-atstor < stor && freq[0]+atstor > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){ genkendtTone = '1';}
        if (freq[5]-at < lille && freq[5]+at > lille){ genkendtTone = '4';}
        if (freq[6]-at < lille && freq[6]+at > lille){ genkendtTone = '7';}
        if (freq[7]-at < lille && freq[7]+at > lille){ genkendtTone = '*';}
    }
    else if(freq[1]-atstor < stor && freq[1]+atstor > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){ genkendtTone = '2';}
        if (freq[5]-at < lille && freq[5]+at > lille){ genkendtTone = '5';}
        if (freq[6]-at < lille && freq[6]+at > lille){ genkendtTone = '8';}
        if (freq[7]-at < lille && freq[7]+at > lille){ genkendtTone = '0';}
    }
    else if(freq[2]-atstor < stor && freq[2]+atstor > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){ genkendtTone = '3';}
        if (freq[5]-at < lille && freq[5]+at > lille){ genkendtTone = '6';}
        if (freq[6]-at < lille && freq[6]+at > lille){ genkendtTone = '9';}
        if (freq[7]-at < lille && freq[7]+at > lille){ genkendtTone = '#';}
    }
    else if(freq[3]-atstor < stor && freq[3]+atstor > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){ genkendtTone = 'A';}
        if (freq[5]-at < lille && freq[5]+at > lille){ genkendtTone = 'B';}
        if (freq[6]-at < lille && freq[6]+at > lille){ genkendtTone = 'C';}
        if (freq[7]-at < lille && freq[7]+at > lille){ genkendtTone = 'D';}
    } 
    else{

        genkendtTone = 'm'; // m for mellemrum i vektoren.

        //til timeren reset tid fordi hvis den ikke kan genkende dem så er der et mix af 2 buffere.
        if(TimingBool){// siger at vi har set start flaget 
            NyTid = true;
        }
    }

    //fjerner et underligt NULL
    if((int)genkendtTone == 0){
        return;
    }

    std::cout << genkendtTone << std::endl;
}



void computingFFT(char & genkendtTone){
    fftw_execute(Data->plan); //gør fft'en. Nu er out fyldt med data som er transformet af fft'en
    
    int storsteAmplitude = -1;
    int andenAmplitude = -1;

    for(int i = 0; i < FRAMES_PER_BUFFER; i++){ //Kun igennem halvdelen (/2) pga spejlning i resultatet.
        if(Data->out[i] * Data->out[i] > Data->out[storsteAmplitude] * Data->out[storsteAmplitude]){ //magnitude, hvor kvadratroden er "sparet" væk
            if(( (double(i) * SAMPLE_RATE / FRAMES_PER_BUFFER/2) > 1100) && ((double(i) * SAMPLE_RATE / FRAMES_PER_BUFFER/2) < 1700) ){
                storsteAmplitude = i;
            }
        }
        if(Data->out[i] * Data->out[i] > Data->out[andenAmplitude] * Data->out[andenAmplitude]){ //magnitude, hvor kvadratroden er "sparet" væk
            if(( (double(i) * SAMPLE_RATE / FRAMES_PER_BUFFER/2) > 600) && ((double(i) * SAMPLE_RATE / FRAMES_PER_BUFFER/2) < 1000) ){
                andenAmplitude = i;
            }
        }
    }
    double storsteFrekvens = (double)storsteAmplitude * SAMPLE_RATE / FRAMES_PER_BUFFER/2; //Vi dividere med 2 pga. spejlning af resultatet.
    double andenFrekvens = (double)andenAmplitude * SAMPLE_RATE / FRAMES_PER_BUFFER/2;
    
    genkendDTMFtoner(storsteFrekvens, andenFrekvens, genkendtTone);
}


int teller =0;
int portAudioBufferTid = 21; //mili sekunder //målt tid for påfyldningen
int bufferoptagningslaengde = 63; //mili sekunder
int64_t afspilningsTid = (portAudioBufferTid*7) - 2; //Bufferen skal være 63 så passer tiden, Den tager oftere 1 sekundt mere end under så den rammer 63.


vector<char> tonerEnTid;
vector<char> alleTonerVector;
char genkendtTone;

void FFT() {
    while((callbackInfo.size() >= (FRAMES_PER_BUFFER)) && (!callbackInfo.empty())){
        
        for(int i = 0; i <FRAMES_PER_BUFFER; i++){
            Data->in[i]= callbackInfo.front(); //tager index 0.
            callbackInfo.pop(); //sletter idex 0, index 1 bliver ny index 0
        }
        
        computingFFT(genkendtTone);

        freqVals.push_back(genkendtTone);

        checkForFlag(freqVals);


        if(TimingBool && genkendtTone != 'm'){
            tonerEnTid.push_back(genkendtTone);
        }

        auto CurrentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(CurrentTime - startTime).count();
        if(((elapsedTime > (afspilningsTid*1000)) && TimingBool) || NyTid){ // *1000 fordi tjek er i mili sekunder.
            NyTid = false;

            startTime = std::chrono::high_resolution_clock::now();
            teller++;
            std::cout << "                      Ny tid " << teller << std::endl;


            if((teller > 0) && (tonerEnTid.size() > 0)){

                alleTonerVector.push_back(getMaxOccurringChar(tonerEnTid));

                tonerEnTid.clear();
            }
            else{
                tonerEnTid.clear();
            }
        }
    }
}



static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData){
    
    paTestData *data = (paTestData *)userData; //??????

    (void)outputBuffer;//Disse skal ikke retunere noget. Vi bruger dem ikke. 
    (void)timeInfo;
    (void)statusFlags;

    /*En pointer til addressen for inputbufferen (den optagne lyd) i formatet en double.*/
    double *in = (double*)inputBuffer; 


    auto OverstTid = std::chrono::high_resolution_clock::now();
    auto elapsedTimeTWO = std::chrono::duration_cast<std::chrono::microseconds>( OverstTid - nuTid).count();

    if((elapsedTimeTWO > 18000) || !TimingBool){ //er syncroniseret

        nuTid = std::chrono::high_resolution_clock::now();
        for (unsigned long i = 0; i < FRAMES_PER_BUFFER; i++) {
            callbackInfo.push(in[i]); //overføre til vores "ringe buffer"
        }
        FFT();
    }

    if (data->finished == paComplete){
        return paComplete;
    }

/*Retunerer finished, som enten retunere "paContinue" (forsæt optagelse) eller "paComplete" (færdig med optagelse).*/
    return paContinue;
}




void toneToBit(vector<char> &Tones, vector<string> &BitSets){
    string Placeholder;
    for (int i = 0; i < Tones.size(); i++){
        if(Tones[i]== '1'){Placeholder = ("0000");}
        if(Tones[i]== '2'){Placeholder = ("0001");}
        if(Tones[i]== '3'){Placeholder = ("0010");}
        if(Tones[i]== 'A'){Placeholder = ("0011");}
        if(Tones[i]== '4'){Placeholder = ("0100");}
        if(Tones[i]== '5'){Placeholder = ("0101");}
        if(Tones[i]== '6'){Placeholder = ("0110");}
        if(Tones[i]== 'B'){Placeholder = ("0111");}
        if(Tones[i]== '7'){Placeholder = ("1000");}
        if(Tones[i]== '8'){Placeholder = ("1001");}
        if(Tones[i]== '9'){Placeholder = ("1010");}
        if(Tones[i]== 'C'){Placeholder = ("1011");}
        if(Tones[i]== '*'){Placeholder = ("1100");}
        if(Tones[i]== '0'){Placeholder = ("1101");}
        if(Tones[i]== '#'){Placeholder = ("1110");}
        if(Tones[i]== 'D'){Placeholder = ("1111");}
        BitSets.push_back(Placeholder);
    }    
}


void bitToByte(vector<string> &BitsetstoByte){
    vector<string> tempvec;
    for (int i = 0; i < BitsetstoByte.size(); i+=2){ //takes every bitset and combines them to a byte
        string string1 = BitsetstoByte[i]+BitsetstoByte[i+1];
        tempvec.push_back(string1);
    }
    BitsetstoByte=tempvec;
}

void byteToAscii(string &asciiString,vector<string> &byte){
    string Placeholder;
    for (int i = 0; i < byte.size(); i++){
        int asciiVal=bitset<8>(byte[i]).to_ulong(); //takes the byte and converts it into a ascii char
        Placeholder.push_back(static_cast<char>(asciiVal));
    }
    asciiString=Placeholder;
}


void toneToAscii(vector<char> &input,string &output){
vector<string> placeholder;
toneToBit(input,placeholder);
bitToByte(placeholder);
byteToAscii(output,placeholder);
}




int main(void){
    PaStreamParameters inputParameters;
    PaStream *stream;
    PaError err;

    err = Pa_Initialize();
    checkErr(err);

    Data = (paTestData*)fftw_malloc(sizeof(paTestData));
    Data->in = (double*)fftw_malloc(sizeof(double) * FRAMES_PER_BUFFER);
    Data->out = (double*)fftw_malloc(sizeof(double) * FRAMES_PER_BUFFER);
    Data->plan = fftw_plan_r2r_1d(FRAMES_PER_BUFFER, Data->in, Data->out, FFTW_R2HC, FFTW_ESTIMATE);


    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    if (inputParameters.device == paNoDevice){
        fprintf(stderr, "Error: No default input device.\n");
    }
    checkErr(err);
    
    inputParameters.channelCount = 1; /* Mono input */
    inputParameters.sampleFormat = paFloat32; // Sample format er float
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream( 
        &stream,
        &inputParameters,
        NULL, /* &outputParameters, */
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paNoFlag,   
        recordCallback,
        Data);
    checkErr(err);

    err = Pa_StartStream(stream);
    checkErr(err);

    std::cout << "\n=== Now recording!! ===\n" << std::endl;

    while ((err = Pa_IsStreamActive(stream)) == 1){ //Optager intil array er udfyldt
    }

    checkErr(err);
    err = Pa_CloseStream(stream);
    checkErr(err);

    std::cout << "\n=== Done recording!! ===" << std::endl << std::endl;

    Pa_Terminate();
    checkErr(err);
    
    cout << "Antal værdier: " << alleTonerVector.size() << endl << endl;

    cout << "Toner:" << endl;
    for (int i = 0; i < alleTonerVector.size(); i++){
        cout << alleTonerVector[i] << ",";
        if(0 == ((i+1)%2)){cout << " ";}
    }
    cout << endl;


    cout << "Tone To Ascii: " << endl;
    string output;
    toneToAscii(alleTonerVector, output);
    output;


    fftw_destroy_plan(Data->plan);
    fftw_free(Data->in);
    fftw_free(Data->out);
    fftw_free(Data);
    fftw_cleanup();

    return 0;
}