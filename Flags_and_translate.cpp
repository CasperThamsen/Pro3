#include <iostream>
#include <bitset>
#include <string>
#include <vector>
using namespace std;

void asciitobit(char input, bitset<8> &x){
    // Input character
    char inputChar = input;
    unsigned char byteValue = static_cast<unsigned char>(inputChar);
    // Display the result as an 8-bit binary string
    bitset<8> binaryRepresentation(byteValue);
    x = binaryRepresentation;
}

void bittotone(string input, char &tone)
{
    if (input == string("0000")){tone = '1';}
    if (input == string("0001")){tone = '2';}
    if (input == string("0010")){tone = '3';}
    if (input == string("0011")){tone = 'A';}
    if (input == string("0100")){tone = '4';}
    if (input == string("0101")){tone = '5';}
    if (input == string("0110"))
    {
        tone = '6';
    }
    if (input == string("0111"))
    {
        tone = 'B';
    }
    if (input == string("1000"))
    {
        tone = '7';
    }
    if (input == string("1001"))
    {
        tone = '8';
    }
    if (input == string("1010"))
    {
        tone = '9';
    }
    if (input == string("1011"))
    {
        tone = 'C';
    }
    if (input == string("1100"))
    {
        tone = '*';
    }
    if (input == string("1101"))
    {
        tone = '0';
    }
    if (input == string("1110"))
    {
        tone = '#';
    }
    if (input == string("1111"))
    {
        tone = 'D';
    }
}
void stringtotone(string import, vector<char> &x)
{
    for (int i = 0; i < import.length(); i++)
    {
        bitset<8> BitSet;
        char input = import[i];
        asciitobit(input, BitSet);
        string test = BitSet.to_string();
        string bit1;
        string bit2;
        char tone1;
        char tone2;
        for (int j = 0; j < 4; j++)
        {
            bit1 += test[j];
        }
        for (int k = 4; k < 8; k++)
        {
            bit2 += test[k];
        }
        bittotone(bit1, tone1);
        bittotone(bit2, tone2);
        x.push_back(tone1);
        x.push_back(tone2);
    }
}
void tonetobit(vector<char> &x, vector<string> &y)
{
string string1;
    for (int i = 0; i < x.size(); i++)
    {
        if(x[i]== '1')
        {
        string1 = ("0000");
        }
        if(x[i]== '2')
        {
        string1 = ("0001");
        }
        if(x[i]== '3')
        {
        string1 = ("0010");
        }
        if(x[i]== 'A')
        {
        string1 = ("0011");
        }
        if(x[i]== '4')
        {
        string1 = ("0100");
        }
        if(x[i]== '5')
        {
        string1 = ("0101");
        }
        if(x[i]== '6')
        {
        string1 = ("0110");
        }
        if(x[i]== 'B')
        {
        string1 = ("0111");
        }
        if(x[i]== '7')
        {
        string1 = ("1000");
        }
        if(x[i]== '8')
        {
        string1 = ("1001");
        }
        if(x[i]== '9')
        {
        string1 = ("1010");
        }
        if(x[i]== 'C')
        {
        string1 = ("1011");
        }
        if(x[i]== '*')
        {
        string1 = ("1100");
        }
        if(x[i]== '0')
        {
        string1 = ("1101");
        }
        if(x[i]== '#')
        {
        string1 = ("1110");
        }
        if(x[i]== 'D')
        {
        string1 = ("1111");
        }
        y.push_back(string1);
    }    
}
void bittobyte(vector<string> &y)
{
    vector<string> tempvec;
    for (int i = 0; i < y.size(); i+=2)
    {
        string string1 = y[i]+y[i+1];
        tempvec.push_back(string1);
    }
    y=tempvec;
}
void bytetoascii(string &x,vector<string> &y)
{
string asciisttring;
    for (int i = 0; i < y.size(); i++)
    {
        int asciival=bitset<8>(y[i]).to_ulong();
        asciisttring.push_back(static_cast<char>(asciival));
    }
x=asciisttring;
}
void tonetoascii(vector<char> &x,string &y)
{
vector<string> placeholder;
tonetobit(x,placeholder);
bittobyte(placeholder);
bytetoascii(y,placeholder);
}
void checkfordupes(vector<char> &x)
{
for (int i = 0; i < x.size(); i++)
{
    vector<char> flag;
    flag.push_back('7');
    flag.push_back('2');
    if (x[i]==x[i+1]){
    x.insert(x.begin()+i+1,flag.begin(),flag.end());
    i=i+2;
    }
    if (x[i]=='7' && x[i+1]=='2'){
        x.insert(x.begin()+i,flag.begin(),flag.end());
    }

    
    
}

}


void removecheckfordupes(vector<char> &x){
    for (int i = 0; i < x.size(); i++){
        if(x[i]=='7'&& x[i+1]=='2')
        x.erase(x.begin()+i,x.begin()+i+2);
    }    
}

void insertflags(vector<char> &x){
    std::vector<char> flag;
    flag.push_back('3');
    flag.push_back('A');
    std::vector<char> endflag;
    endflag.push_back('3');
    endflag.push_back('4');

    for (int i = 0; i < 4; i++){
        x.insert(x.begin(), flag.begin(), flag.end());
        x.insert(x.end(), endflag.begin(), endflag.end());
    }
}


void checkcommand(vector<char> &x, string &y){
    vector<char> test;
    if (x.size()>=6){
        for (int i = 0; i < 6; i++){
            test.push_back(x[i]);
        }   
    }else{
        cout<<"Failure"<<endl;
    }
    tonetoascii(test,y);
}


int main(){
    vector<char> BitSekvens;
    string Kommandotest;
    string import = "2$2$2$2$2$2$";
    stringtotone(import,BitSekvens);
    for (int i = 0; i < BitSekvens.size(); i++){
        cout<<BitSekvens[i]<<endl;
    }
    
    checkcommand(BitSekvens,Kommandotest);
    cout<<Kommandotest<<endl;
    return 0;
}
