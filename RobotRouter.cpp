#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
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

#define SAMPLE_RATE (8000)
#define TABLE_SIZE (8000)

using namespace std;

/////////////////////////////////////ROS2//////////////////////////////////////////////////////
void routing_table(auto node, std::string addresseString){
    std::vector<float> timeVec;
    std::vector<float> linareDistVec;
    std::vector<float> rotationDistVec;

    if(addresseString == "123"){ //laver lyn
        timeVec = {(1 / 0.1), (3.14/2 / 1), (0.5 / 0.1), (3.14/2 / 1), (1 / 0.1)};
        linareDistVec = {0.1,0,0.1,0,0.1};
        rotationDistVec = {0,-1,0,1,0};
    }
    else if(addresseString == "000"){ //kører lige ud i 1 menter
        timeVec = {(1 / 0.1)};
        linareDistVec = {0.1};
        rotationDistVec = {0};
    }
    else{ //Drejer 360 rundt om sig selv
        timeVec = {((3.14*2) / 1)}; // 2 radianer
        linareDistVec = {0};
        rotationDistVec = {1};
    }

    std::cout << "addresseString ros2 " << addresseString << std::endl;

    for(int i = 0; i < timeVec.size(); i++){
        node->publish_vel(linareDistVec[i],rotationDistVec[i]);
        int timeToWait = timeVec[i] * 1000;
        std::this_thread::sleep_for(std::chrono::milliseconds(timeToWait));
    }
    node->publish_vel(0,0);
}

class RB3_cpp_publisher : public rclcpp::Node{
  public:
    //Create publisher for publishing veloctiy commands to topic "cmd_vel"
    RB3_cpp_publisher()
    : Node("rb3_cpp_publisher"){
      publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel",10);
    }

  //Class function to be called when you want to publish velocity commands
  void publish_vel(float lin_vel_cmd, float ang_vel_cmd){
    // Set angular velocity to desired value (ie. turning)
    msg.angular.x = 0;
    msg.angular.y = 0;
    msg.angular.z = ang_vel_cmd;

    // Set linear velocity to desired value (ie. forward and backwards)
    msg.linear.x = lin_vel_cmd;
    msg.linear.y = 0;
    msg.linear.z = 0;
    RCLCPP_INFO(this->get_logger(), "Publishing: %f , %f", this->msg.linear.x, this->msg.angular.z);
    publisher_->publish(msg);
  }

  private:
  // Private variables used for the publisher
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;
  geometry_msgs::msg::Twist msg;
};

/////////////////////////////////////ROS2//////////////////////////////////////////////////////

/////////////////////////////////////RECORD//////////////////////////////////////////////////////
typedef struct{
    double* in;      // Input buffer, will contain our audio sample
    double* out;
    fftw_plan plan;
    int finished = paContinue;
} paRecordData;

static paRecordData* Data; //opretter den globalt

vector<char> freqVals; 

queue<double> callbackInfo; // kan tænkes som en vektor buffer der kan fjernes forfra
bool TimingBool = false; //checker i FFT
auto startTime = std::chrono::high_resolution_clock::now();

int timeCount = 0;
const int bufferTime = 40;
int recordingTime = (bufferTime*6) - 2; //Bufferen skal være 240

vector<char> tonesCurrentTime;
vector<char> everyToneVector;
char toneCharfound;
/////////////////////////////////////RECORD SLUT//////////////////////////////////////////////////////

/////////////////////////////////////AFSPIL//////////////////////////////////////////////////////
typedef struct{
    float dtmfArray[16][TABLE_SIZE];  //16
    unsigned int toneToPlay;
    unsigned int phase;
} paDataAudio;


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
/////////////////////////////////////AFSPIL SLUT//////////////////////////////////////////////////////


static void checkErr(PaError err){
    if(err != paNoError){
        std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void removeRepeatedTones(vector<char> & CharVector){   
    for (int i = 0; i < CharVector.size()-1; i++){
        if(CharVector[i] == CharVector[i+1]){
            CharVector.erase(CharVector.begin()+i);
            --i;
        }
    }
}


bool checkPattern(string str, string pattern){
//https://www.geeksforgeeks.org/check-string-follows-order-characters-defined-pattern-not/
    // len stores length of the given pattern 
    int len = pattern.length();
      
    // if length of pattern is more than length of 
    // input string, return false;
    if (str.length() < len){
        return false;
    }
      
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
        if (last == string::npos || first ==  string::npos || last > first){
            return false;
        }
    }

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

void checkForStartFlag(vector<char> CharVector) {
    const std::string startFlag = "159D";
    const int lookBack = 30; 
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
    if((CharVector.size() >= lookBack)){
        std::string lastTones(CharVector.end() - lookBack, CharVector.end());
        if (checkPattern(lastTones, stopflag)) {
            std::cout << "stop flag detected!" << std::endl;
            Data->finished = paComplete;
        }
    }
}


void genkendDTMFtoner(double stor, double lille, char& toneCharfound){     
    double at = 25; //Tilladt afvigelse
    double atstor = 25; 

    std::vector<double> freq{1209, 1336, 1477, 1633, 679, 770, 852, 941};

    if(freq[0]-atstor < stor && freq[0]+atstor > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){ toneCharfound = '1';}
        if (freq[5]-at < lille && freq[5]+at > lille){ toneCharfound = '4';}
        if (freq[6]-at < lille && freq[6]+at > lille){ toneCharfound = '7';}
        if (freq[7]-at < lille && freq[7]+at > lille){ toneCharfound = '*';}
    }
    else if(freq[1]-atstor < stor && freq[1]+atstor > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){ toneCharfound = '2';}
        if (freq[5]-at < lille && freq[5]+at > lille){ toneCharfound = '5';}
        if (freq[6]-at < lille && freq[6]+at > lille){ toneCharfound = '8';}
        if (freq[7]-at < lille && freq[7]+at > lille){ toneCharfound = '0';}
    }
    else if(freq[2]-atstor < stor && freq[2]+atstor > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){ toneCharfound = '3';}
        if (freq[5]-at < lille && freq[5]+at > lille){ toneCharfound = '6';}
        if (freq[6]-at < lille && freq[6]+at > lille){ toneCharfound = '9';}
        if (freq[7]-at < lille && freq[7]+at > lille){ toneCharfound = '#';}
    }
    else if(freq[3]-atstor < stor && freq[3]+atstor > stor){
        if (freq[4]-at < lille && freq[4]+at > lille){ toneCharfound = 'A';}
        if (freq[5]-at < lille && freq[5]+at > lille){ toneCharfound = 'B';}
        if (freq[6]-at < lille && freq[6]+at > lille){ toneCharfound = 'C';}
        if (freq[7]-at < lille && freq[7]+at > lille){ toneCharfound = 'D';}
    } 
    else{
        toneCharfound = 'm'; // m for mellemrum i vektoren.
    }

    //fjerner et underligt NULL
    if((int)toneCharfound == 0){
	cout<<"Null Fundet"<<endl;
        return;
    }

    std::cout << toneCharfound << std::endl;
}

void computingFFT(char & toneCharfound){
    fftw_execute(Data->plan); //gør fft'en. Nu er out fyldt med data som er transformet af fft'en
    
    int storsteAmplitude = -1;
    int andenAmplitude = -1;

    for(int i = 0; i < FRAMES_PER_BUFFER_RECORD; i++){ //Kun igennem halvdelen (/2) pga spejlning i resultatet.
        if(Data->out[i] * Data->out[i] > Data->out[storsteAmplitude] * Data->out[storsteAmplitude]){ //magnitude, hvor kvadratroden er "sparet" væk
            if(( (double(i) * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD/2) > 1100) && ((double(i) * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD/2) < 1700) ){
                storsteAmplitude = i;
            }
        }
        if(Data->out[i] * Data->out[i] > Data->out[andenAmplitude] * Data->out[andenAmplitude]){ //magnitude, hvor kvadratroden er "sparet" væk
            if(( (double(i) * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD/2) > 600) && ((double(i) * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD/2) < 1000) ){
                andenAmplitude = i;
            }
        }
    }
    double storsteFrekvens = (double)storsteAmplitude * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD/2; //Vi dividere med 2 pga. spejlning af resultatet.
    double andenFrekvens = (double)andenAmplitude * SAMPLE_RATE_RECORD / FRAMES_PER_BUFFER_RECORD/2;

    genkendDTMFtoner(storsteFrekvens, andenFrekvens, toneCharfound);
}

void FFT(){
    while((callbackInfo.size() >= (FRAMES_PER_BUFFER_RECORD)) && (!callbackInfo.empty())){
        
        for(int i = 0; i <FRAMES_PER_BUFFER_RECORD; i++){
            Data->in[i]= callbackInfo.front(); //tager index 0.
            callbackInfo.pop(); //sletter idex 0, index 1 bliver ny index 0
        }
        
        computingFFT(toneCharfound);
        freqVals.push_back(toneCharfound);

        if(TimingBool && toneCharfound != 'm'){
            tonesCurrentTime.push_back(toneCharfound);
        }

        if(!TimingBool){
            checkForStartFlag(freqVals);
        }
        else{
            checkForStopFlag(everyToneVector);
        }

        auto CurrentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(CurrentTime - startTime).count();
        if(((elapsedTime > (recordingTime*1000)) && TimingBool)){ // *1000 fordi tjek er i mili sekunder.

            startTime = std::chrono::high_resolution_clock::now();
            timeCount++;
            std::cout << "                      Ny tid " << timeCount << std::endl;

            if((timeCount > 0) && (tonesCurrentTime.size() > 0)){
                everyToneVector.push_back(getMaxOccurringChar(tonesCurrentTime));
                tonesCurrentTime.clear();
            }
            else{
                tonesCurrentTime.clear();
            }
        }
    }
}

static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData){
    
    paRecordData *data = (paRecordData *)userData;

    (void)outputBuffer;//Disse skal ikke retunere noget. Vi bruger dem ikke. 
    (void)timeInfo;
    (void)statusFlags;

    //En pointer til addressen for inputbufferen (den optagne lyd) i formatet en double.
    double *in = (double*)inputBuffer; 

    for (unsigned long i = 0; i < FRAMES_PER_BUFFER_RECORD; i++) {
        callbackInfo.push(in[i]); //overføre til vores queue
    }
    FFT();

    if (data->finished == paComplete){
        return paComplete;
    }

//Retunerer finished, som enten retunere "paContinue" (forsæt optagelse) eller "paComplete" (færdig med optagelse).
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

void checkCommand(vector<char> &inputTones, string &Kommandooutput){
    vector<char> TempVec;
    if (inputTones.size()>=6){
        for (int i = 0; i < 6; i++){
            TempVec.push_back(inputTones[i]);
        }
        for (int i = 0; i < 6; i++){ //fjerne de 6 forreste toner så vi ikke sender dem videre
            inputTones.erase(inputTones.begin());
        }
    
    }else{
        cout<<"Failure"<<endl;
    }
    toneToAscii(TempVec,Kommandooutput);
}


////////////////////////////afspilnings callback////////////////////////////////////////
static int audioCallback(const void* inputBuffer, void* outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData){
    paDataAudio* data = (paDataAudio*)userData;
    float* out = (float*)outputBuffer;

    (void)timeInfo;
    (void)statusFlags;
    (void)inputBuffer;

    static unsigned long i;
    for (i = 0; i < framesPerBuffer; i++){

        *out++ = data->dtmfArray[data->toneToPlay][data->phase];

        data->phase +=  1;
        if( data->phase >= TABLE_SIZE ) data->phase -= TABLE_SIZE; //fjerner skrætten
    }

    return paContinue;
}
////////////////////////////afspilnings callback////////////////////////////////////////


int main(int argc, char ** argv){
    PaStreamParameters inputParameters, outputParametersAudio;
    PaStream *stream;
    PaError err;

    paDataAudio dataAudio;

    err = Pa_Initialize();
    checkErr(err);

    Data = (paRecordData*)fftw_malloc(sizeof(paRecordData));
    Data->in = (double*)fftw_malloc(sizeof(double) * FRAMES_PER_BUFFER_RECORD);
    Data->out = (double*)fftw_malloc(sizeof(double) * FRAMES_PER_BUFFER_RECORD);
    Data->plan = fftw_plan_r2r_1d(FRAMES_PER_BUFFER_RECORD, Data->in, Data->out, FFTW_R2HC, FFTW_ESTIMATE);

    inputParameters.device = 6; //usb mic nr
    
    inputParameters.channelCount = 1; // Mono input 
    inputParameters.sampleFormat = paFloat32; // Sample format er float
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream( 
        &stream,
        &inputParameters,
        NULL, // &outputParameters, 
        SAMPLE_RATE_RECORD,
        FRAMES_PER_BUFFER_RECORD,
        paNoFlag,   
        recordCallback,
        Data);
    checkErr(err);

    err = Pa_StartStream(stream);
    checkErr(err);

    std::cout << "\n=== Now recording!! ===\n" << std::endl;

    while ((err = Pa_IsStreamActive(stream)) == 1){ //Optager intil paComplete
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

    // printer til terminalen for error check
    cout << "Antal værdier: " << everyToneVector.size() << endl << endl;
    cout << "Toner:" << endl;
    for (int i = 0; i < everyToneVector.size(); i++){
        cout << everyToneVector[i] << ",";
        if(0 == ((i+1)%2)){cout << " ";}
    }
    cout << endl;

    string output;
    toneToAscii(everyToneVector, output);
    cout << "Anden computer siger: " << output << endl;

//////
//robotten skal køre her

    string AddresseString;
    checkCommand(everyToneVector, AddresseString);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); //giver den lige lidt tid fra optag til kør

    rclcpp::init(argc, argv);
    auto vel_node = std::make_shared<RB3_cpp_publisher>();
    routing_table(vel_node, AddresseString);

///output skal afspilles her:
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
                dataAudio.dtmfArray[j][i] = sin1 + sin2;
            }
            j++;
        }
    }

    dataAudio.phase = 0;

    outputParametersAudio.device = 6;
    outputParametersAudio.channelCount = 1;        // Mono output
    outputParametersAudio.sampleFormat = paFloat32; // 32-bit floating-point output
    outputParametersAudio.suggestedLatency = Pa_GetDeviceInfo(outputParametersAudio.device)->defaultLowOutputLatency;
    outputParametersAudio.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream,
    NULL,
    &outputParametersAudio,
    SAMPLE_RATE,
    paFramesPerBufferUnspecified,
    paClipOff,
    audioCallback,
    &dataAudio);
    checkErr(err);

//Vector of integers (startflag:159D and endflag:2*BD)
    vector<int> vectorStartFlag = {0,5,10,15};
    vector<int> vectorEndFlag = {1,12,7,15};
    vector<int> vectorIntToBePlayed = vectorCharToVectorInt(everyToneVector);

//Starting streamAudio
    err = Pa_StartStream(stream);
    checkErr(err);

    int playingTime = (21*6); // bufferTime er 40

//Afspiller startflag: 1 5 9 D
    for(int i = 0; i < vectorStartFlag.size(); i++){
        dataAudio.toneToPlay = vectorStartFlag[i];
        std::this_thread::sleep_for(std::chrono::milliseconds(playingTime));
    }

//Afspiller selve sekvensen
    for(int i = 0; i < vectorIntToBePlayed.size(); i++){ //Looper igennem de indtastede chars's toner.
        dataAudio.toneToPlay = vectorIntToBePlayed[i]; //Afspil input vektoren
        std::this_thread::sleep_for(std::chrono::milliseconds(playingTime));
    }

//Afspiller endflag: 2 * B D
    for (int i = 0; i < vectorEndFlag.size(); i++){
        dataAudio.toneToPlay = vectorEndFlag[i];
        std::this_thread::sleep_for(std::chrono::milliseconds(playingTime));
    }

//Stoping streamAudio
    err = Pa_StopStream(stream);
    checkErr(err);

    err = Pa_CloseStream(stream);
    checkErr(err);

    Pa_Terminate();
    checkErr(err);
    
    //ros2 publisher Shutdown node when complete
    rclcpp::shutdown();

    return 0;
}
