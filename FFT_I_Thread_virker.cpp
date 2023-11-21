#include <iostream>
#include <string>

#include "portaudio.h"
#include "fftw3.h"


#include <complex>
#include <chrono>
#include <thread>
#include <queue>

#define SAMPLE_RATE (44100.0)
#define FRAMES_PER_BUFFER (512) //Normalt 1024 eller 512

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

bool FFTThreadBool = true;
queue<double> callbackInfo; //"ringbuffer" agtig kan tænkes som en vektor buffer der kan fjernes for fra


static void checkErr(PaError err){
    if(err != paNoError){
        std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void genkendDTMFtoner(double stor, double lille){ 
    double at = 35; //Tilladt afvigelse
    std::vector<double> freq{1209, 1336, 1477, 1633, 679, 770, 852, 941};

    if(freq[0]-at < stor && freq[0]+at > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){std::cout << "1" << std::endl; freqVals.push_back('1');}
        if (freq[5]-at < lille && freq[5]+at > lille){std::cout << "4" << std::endl; freqVals.push_back('4');}
        if (freq[6]-at < lille && freq[6]+at > lille){std::cout << "7" << std::endl; freqVals.push_back('7');}
        if (freq[7]-at < lille && freq[7]+at > lille){std::cout << "*" << std::endl; freqVals.push_back('*');}
    }

    if(freq[1]-at < stor && freq[1]+at > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){std::cout << "2" << std::endl; freqVals.push_back('2');}
        if (freq[5]-at < lille && freq[5]+at > lille){std::cout << "5" << std::endl; freqVals.push_back('5');}
        if (freq[6]-at < lille && freq[6]+at > lille){std::cout << "8" << std::endl; freqVals.push_back('8');}
        if (freq[7]-at < lille && freq[7]+at > lille){std::cout << "0" << std::endl; freqVals.push_back('0');}
    }

    if(freq[2]-at < stor && freq[2]+at > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){std::cout << "3" << std::endl; freqVals.push_back('3');}
        if (freq[5]-at < lille && freq[5]+at > lille){std::cout << "6" << std::endl; freqVals.push_back('6');}
        if (freq[6]-at < lille && freq[6]+at > lille){std::cout << "9" << std::endl; freqVals.push_back('9');}
        if (freq[7]-at < lille && freq[7]+at > lille){std::cout << "#" << std::endl; freqVals.push_back('#');}
    }

    if(freq[3]-at < stor && freq[3]+at > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){std::cout << "A" << std::endl; freqVals.push_back('A');}
        if (freq[5]-at < lille && freq[5]+at > lille){std::cout << "B" << std::endl; freqVals.push_back('B');}
        if (freq[6]-at < lille && freq[6]+at > lille){std::cout << "C" << std::endl; freqVals.push_back('C');}
        if (freq[7]-at < lille && freq[7]+at > lille){std::cout << "D" << std::endl; freqVals.push_back('D');
            Pa_Sleep(200); //Er ikke 100% sikker på hvorfor det virker, men en aprubt paComplete giver fejl!
            //måske vi i FFTComputing skal lave en special case der tager sig af nå end flag er nået så man kan free memory rigtigt
            Data->finished = paComplete;
        }
    }
}

void removeRepeatedTones(){   
    for (int i = 0; i < freqVals.size()-1; i++){
        if(freqVals[i] == freqVals[i+1]){
            freqVals.erase(freqVals.begin()+i);
            --i;
        }
    }
    cout << "Toner:" << endl;
    for (int i = 0; i < freqVals.size(); i++){
        cout << freqVals[i] << ",";
        if(0 == ((i+1)%2)){cout << " ";}
    }
    cout << endl;
}


void FFTComputing() {
    while(FFTThreadBool) {
        if(!callbackInfo.empty()){
            while(((callbackInfo.size() >= FRAMES_PER_BUFFER) && (!callbackInfo.empty()))){
                for(int i = 0; i <FRAMES_PER_BUFFER; i++){
                    Data->in[i]= callbackInfo.front(); //tager index 0.
                    callbackInfo.pop(); //sletter idex 0, index 1 bliver ny index 0.|| (callbackInfo.size() == (FRAMES_PER_BUFFER)
                }

                fftw_execute(Data->plan); //gør fft'en. Nu er out fyldt med data som er transformet af fft'en
                
                int storsteAmplitude = -1;
                int andenAmplitude = -1;

                for(unsigned long i = 0; i < FRAMES_PER_BUFFER; i++){
                    if((Data->out[i] * Data->out[i]) > Data->out[storsteAmplitude] * Data->out[storsteAmplitude]){ //magnitude, hvor kvadratroden er "sparet" væk
                        andenAmplitude = storsteAmplitude;
                        storsteAmplitude = i;
                    }
                }
                double storsteFrekvens = (double)storsteAmplitude * SAMPLE_RATE / FRAMES_PER_BUFFER/2; //Vi dividere med 2 pga. spejlning af resultatet.
                double andenFrekvens = (double)andenAmplitude * SAMPLE_RATE / FRAMES_PER_BUFFER/2;
                
                genkendDTMFtoner(storsteFrekvens, andenFrekvens);
            }
        }
        else{
            Pa_Sleep(15); //giver callbacken tid til at overføre til callbackInfo (vores ring buffer)
            //er i stedet for mutex lock men måske vi skal se anden steds. var et hurtigt fix
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

    for (unsigned long i = 0; i < FRAMES_PER_BUFFER; i++) {
        callbackInfo.push(in[i]); //overføre til vores "ringe buffer"
    }

    if (data->finished == paComplete){
        return paComplete;
    }

/*Retunerer finished, som enten retunere "paContinue" (forsæt optagelse) eller "paComplete" (færdig med optagelse).*/
    return paContinue;
}


int main(void){
    PaStreamParameters inputParameters;
    PaStream *stream;
    PaError err;

    std::thread FFTComputingThread(FFTComputing);

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
    
    cout << "Antal værdier: " << freqVals.size() << endl << endl;

    
    removeRepeatedTones(); //printer

    /*Stop FFT*/
    FFTThreadBool = false;
    FFTComputingThread.join();


    fftw_destroy_plan(Data->plan);
    fftw_free(Data->in); //????
    fftw_free(Data->out);
    fftw_free(Data);
    fftw_cleanup();

    return 0;
}
