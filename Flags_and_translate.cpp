#include <iostream>
#include <bitset>
#include <string>
#include <vector>
using namespace std;


void asciiToBit(char input, bitset<8> &x){
    char inputChar = input;
    unsigned char byteValue = static_cast<unsigned char>(inputChar); //change from char to byte in decimal value
    bitset<8> binaryRepresentation(byteValue); // change from byte to 8bit reprensentaion
    x = binaryRepresentation;
}


void bitToTone(string input, char &tone){
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


void stringToTone(string import, vector<char> &Tone){
    for (int i = 0; i < import.length(); i++){
        bitset<8> BitSet;
        char input = import[i]; //Takes every char from the string
        asciiToBit(input, BitSet); //Turns it into a 8bit reprenstation
        string test = BitSet.to_string(); //changes it from Bitset to a string
        string bit1;
        string bit2;
        char tone1;
        char tone2;
        for (int j = 0; j < 4; j++) //Takes the first 4 bits{
            bit1 += test[j];
        }
        for (int k = 4; k < 8; k++) //Takes the last 4 bits{
            bit2 += test[k];
        }
        bitToTone(bit1, tone1); //change the first 4 bits to a tone
        bitToTone(bit2, tone2); //change the last 4 bits to a tone
        Tone.push_back(tone1); 
        Tone.push_back(tone2);
    }
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


void checkForDupes(vector<char> &Tonerepresentation){
    for (int i = 0; i < Tonerepresentation.size(); i++){
        vector<char> flag;
        flag.push_back('7');
        flag.push_back('2');
        if (Tonerepresentation[i]==Tonerepresentation[i+1]){
            Tonerepresentation.insert(Tonerepresentation.begin()+i+1,flag.begin(),flag.end());
            i=i+2;
        }
        if (Tonerepresentation[i]=='7' && Tonerepresentation[i+1]=='2'){
            Tonerepresentation.insert(Tonerepresentation.begin()+i,flag.begin(),flag.end());
        }
    }
}


void removeCheckForDupes(vector<char> &Tonerepresentaion){
    for (int i = 0; i < Tonerepresentaion.size(); i++){
        if(Tonerepresentaion[i]=='7'&& Tonerepresentaion[i+1]=='2')
        Tonerepresentaion.erase(Tonerepresentaion.begin()+i,Tonerepresentaion.begin()+i+2);
    } 
}


void insertFlags(vector<char> &Tonerepresentaion){
    std::vector<char> flag;
    flag.push_back('3');
    flag.push_back('A');
    std::vector<char> endflag;
    endflag.push_back('3');
    endflag.push_back('4');

    for (int i = 0; i < 4; i++){
        Tonerepresentaion.insert(Tonerepresentaion.begin(), flag.begin(), flag.end());
        Tonerepresentaion.insert(Tonerepresentaion.end(), endflag.begin(), endflag.end());
    }
}


void checkCommand(vector<char> &inputTones, string &Kommandooutput){
    vector<char> test;
    if (inputTones.size()>=6){
        for (int i = 0; i < 6; i++){
            test.push_back(inputTones[i]);
        }
    
    }else{
        cout<<"Failure"<<endl;
    }
    toneToAscii(test,Kommandooutput);
}



int main(){
    vector<char> BitSekvens;
    string Kommandotest;
    string import = "2$2 $2$ 2$2 $2$";
    string output;
    stringToTone(import,BitSekvens);
    checkcommand(BitSekvens,Kommandotest);
    cout<<Kommandotest<<endl;
    toneToAscii(BitSekvens,output);
    cout<<output<<endl;
    return 0;
}
