#include <iostream>
#include <string>

#include "portaudio.h"

#include <vector>
#include "fftw3.h"
#include <complex>

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (1024)
using namespace std;

/*Variable*/
typedef struct{
    double *out;
    fftw_plan plan;
    int testCounter = 0;
    int finished = paContinue;
    vector<char> freqVals;
} paTestData;

static void checkErr(PaError err){
    if(err != paNoError){
        std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void genkendDTMFtoner(double stor, double lille, paTestData *data){ 
    double at = 35; //Tilladt afvigelse
    std::vector<double> freq{1209, 1336, 1477, 1633, 679, 770, 852, 941};

    if(freq[0]-at < stor && freq[0]+at > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){std::cout << "1" << std::endl; data->freqVals.push_back('1');}
        if (freq[5]-at < lille && freq[5]+at > lille){std::cout << "4" << std::endl; data->freqVals.push_back('4');}
        if (freq[6]-at < lille && freq[6]+at > lille){std::cout << "7" << std::endl; data->freqVals.push_back('7');}
        if (freq[7]-at < lille && freq[7]+at > lille){std::cout << "*" << std::endl; data->freqVals.push_back('*');}
    }

    if(freq[1]-at < stor && freq[1]+at > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){std::cout << "2" << std::endl; data->freqVals.push_back('2');}
        if (freq[5]-at < lille && freq[5]+at > lille){std::cout << "5" << std::endl; data->freqVals.push_back('5');}
        if (freq[6]-at < lille && freq[6]+at > lille){std::cout << "8" << std::endl; data->freqVals.push_back('8');}
        if (freq[7]-at < lille && freq[7]+at > lille){std::cout << "0" << std::endl; data->freqVals.push_back('0');}
    }

    if(freq[2]-at < stor && freq[2]+at > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){std::cout << "3" << std::endl; data->freqVals.push_back('3');}
        if (freq[5]-at < lille && freq[5]+at > lille){std::cout << "6" << std::endl; data->freqVals.push_back('6');}
        if (freq[6]-at < lille && freq[6]+at > lille){std::cout << "9" << std::endl; data->freqVals.push_back('9');}
        if (freq[7]-at < lille && freq[7]+at > lille){std::cout << "#" << std::endl; data->freqVals.push_back('#');}
    }

    if(freq[3]-at < stor && freq[3]+at > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){std::cout << "A" << std::endl; data->freqVals.push_back('A');}
        if (freq[5]-at < lille && freq[5]+at > lille){std::cout << "B" << std::endl; data->freqVals.push_back('B');}
        if (freq[6]-at < lille && freq[6]+at > lille){std::cout << "C" << std::endl; data->freqVals.push_back('C');}
        if (freq[7]-at < lille && freq[7]+at > lille){std::cout << "D" << std::endl; data->freqVals.push_back('D');
            data->finished = paComplete;
        }
    }
}

void removeRepeatedTones(paTestData *data){   
    for (int i = 0; i < data->freqVals.size()-1; i++){
        if(data->freqVals[i] == data->freqVals[i+1]){
            data->freqVals.erase(data->freqVals.begin()+i);
            --i;
        }
    }
}

static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData)
{
    paTestData *data = (paTestData *)userData; //??????

    (void)outputBuffer; /* Prevent unused variable warnings. */ //Disse skal ikke retunere noget. Vi bruger dem ikke. 
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;

/*En pointer til addressen for inputbufferen (den optagne lyd) i formatet en double.*/
    double *in = (double*)inputBuffer; 

/*FFT*/
    data->out = (double *)malloc(sizeof(double) * FRAMES_PER_BUFFER); //Allocere memory til vores output.
    data->plan = fftw_plan_r2r_1d(FRAMES_PER_BUFFER, in, data->out, FFTW_R2HC, FFTW_ESTIMATE); //laver en plan, vi bruger estimate siden patient og mesaure ikke virker lige pt.
    fftw_execute(data->plan); //Udfører fft'en og fylder out med dataet.

/*Find de to største frekvenser*/
    int storsteAmplitude = -1;
    int andenAmplitude = -1;

    for(int i = 0; i < FRAMES_PER_BUFFER; i++){ //Kun igennem halvdelen (/2) pga spejlning i resultatet.
        if(data->out[i] * data->out[i] > data->out[storsteAmplitude] * data->out[storsteAmplitude]){ //magnitude, hvor kvadratroden er "sparet" væk
            andenAmplitude = storsteAmplitude;
            storsteAmplitude = i;
        }
    }

    double storsteFrekvens = (double)storsteAmplitude * SAMPLE_RATE / FRAMES_PER_BUFFER/2; //Vi dividere med 2 pga. spejlning af resultatet.
    double andenFrekvens = (double)andenAmplitude * SAMPLE_RATE / FRAMES_PER_BUFFER/2;
    

/*Printer*/
    // std::cout << "Nummer: " << data->testCounter << std::endl;
    genkendDTMFtoner(storsteFrekvens, andenFrekvens, data);
    // std::cout << "storsteFrekvens: " << storsteFrekvens << std::endl;
    // std::cout << "andenFrekvens:    " << andenFrekvens << std::endl;
    // std::cout << std::endl;
    data->testCounter++;
    
// /*Stop FFT*/    
//     fftw_destroy_plan(plan);
//     fftw_free(out);
//     fftw_cleanup();

/*Retunerer finished, som enten retunere "paContinue" (forsæt optagelse) eller "paComplete" (færdig med optagelse).*/
    return data->finished;    
}



/*MAIN*/
int main(void)
{
    PaStreamParameters inputParameters;

    PaStream *stream;
    PaError err = paNoError;
    paTestData data;

    err = Pa_Initialize();
    checkErr(err);

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
        paClipOff, /* we won't output out of range samples so don't bother clipping them */
        recordCallback,
        &data);
    checkErr(err);

    err = Pa_StartStream(stream);
    checkErr(err);

    std::cout << "\n=== Now recording!! ===\n" << std::endl;

    while ((err = Pa_IsStreamActive(stream)) == 1){ //Optager intil array er udfyldt
        // Pa_Sleep(10);
    }
    checkErr(err);

    err = Pa_CloseStream(stream);
    checkErr(err);

    std::cout << "\n=== Done recording!! ===\n" << std::endl;

/*Stop FFT*/    
    fftw_destroy_plan(data.plan);
    fftw_free(data.out);
    fftw_cleanup();

    Pa_Terminate();
    checkErr(err);


    cout << "Antal værdier: " << data.freqVals.size() << endl << endl;

    removeRepeatedTones(&data);

    cout << "Toner:" << endl;
    for (int i = 0; i < data.freqVals.size(); i++){
        cout << data.freqVals[i] << ",";
        if(0 == ((i+1)%2)){cout << " ";}
    }
    cout << endl;

    return err;
}
