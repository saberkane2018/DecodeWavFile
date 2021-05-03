#include<iostream>
#include <fstream>
#include <vector>
#include<Windows.h>
#include <bitset>
#include <string>
#include <cstring>
#include <algorithm>
using namespace std;

#pragma warning(disable:4996)


//Wav Header
struct wav_header_t
{
	//MetaData
	char chunkID[4]; 
	unsigned long chunkSize; 
	char format[4]; 
	char subchunk1ID[4]; 
	unsigned long subchunk1Size; 
	unsigned short audioFormat;
	unsigned short numChannels;
	unsigned long sampleRate;
	unsigned long byteRate;
	unsigned short blockAlign;
	unsigned short bitsPerSample;
	char subchunk2ID[4];
	unsigned long subchunk2Size;


};

wav_header_t readHeader(FILE* f) {
	wav_header_t header;
	fread(&header, sizeof(header), 1, f);
	printf("Reading the Wav file header :\n");
	// describes the Wav format requiring two sub-chunks : format & data
	printf("File Type: %s\n", header.chunkID);
	printf("File Size: %ld\n", header.chunkSize);
	printf("Format: %s\n", header.format);
	// the fmt CHUNK describing the format of the sound in the data chunk 
	printf("fmt Chunk ID: %s\n", header.subchunk1ID);
	printf("fmt Chunk Length: %ld\n", header.subchunk1Size);
	printf("Audio Format: %hd\n", header.audioFormat);
	printf("Number of Channels: %hd\n", header.numChannels);
	printf("Sample Rate: %ld\n", header.sampleRate);
	printf("Byte Rate: %ld\n", header.byteRate);
	printf("Block Align: %hd\n", header.blockAlign);
	printf("Bits per Sample: %hd\n", header.bitsPerSample);
	// the data CHUNK containing the size of the data and the raw sound data itself
	printf("data chunk ID : %s\n", header.subchunk2ID);
	printf("data Chunk Length: %ld\n", header.subchunk2Size);
	return header;
}

// size in bytes of 11 bits

const int TOTAL_DATA = 2769; 
const int LEADER_SIZE = 652;
const int ID_SIZE = 2;
const int MESSAGE_SIZE = 64;
const int ENDBLOCK_SIZE = 130;
const int MESSAGE_DATA_SIZE = 30;

struct message
{
	vector<bool> data; // 30 bytes = 30*11 booleans
	vector<bool> checksum; // 1 byte = 11 booleans

	int sumDataValue() {

		int result = 0;
		int i = 0;
		int j = 0;
		while (i < 30 * 11) {
			j = 0;
			while (j < 10) {
				if (j >= 1 && j < 9) {
					result += pow(2, j-1) * (int)data[i];
				}
				
				j++;
				i++;
			}
		}

		return result;
	}
	int checksumValue() {
		int result = 0;
		for (int i = 1; i < 9; i++) {
			result += pow(2, i-1) * (int)checksum[i];
		}
		return result;
	}
};



struct DataContainer
{
	vector<bool> leader; // 652 bytes = 652 * 11 booleans 
	vector<bool> ID; // 2 bytes
	vector<message> rawData; // 64 messages
	vector<bool> lastByte; // 1 byte = 11 booleans
	vector<bool> endBlock; // 130 bytes = 130* 11 booleans


	void ReadToDataContainer(int SignalStartIndex, vector<bool> RightSoundbits) {
		for (int i = SignalStartIndex; i < (SignalStartIndex + (LEADER_SIZE * 11)); i++) {

			leader.push_back(RightSoundbits[i]);
		}
		for (int i = SignalStartIndex + (LEADER_SIZE * 11);
			i < (SignalStartIndex + (LEADER_SIZE * 11) + (ID_SIZE * 11)); i++) {

			ID.push_back(RightSoundbits[i]);
		}


		int i = SignalStartIndex + (LEADER_SIZE * 11) + (ID_SIZE * 11);
		while (i < (SignalStartIndex + (LEADER_SIZE * 11) + (ID_SIZE * 11) +
			(MESSAGE_SIZE * (MESSAGE_DATA_SIZE + 1) * 11))) {
			message* msg = new message();
			int j = 0;
			while (j < 30 * 11) {
				msg->data.push_back(RightSoundbits[i]);
				i++;
				j++;
			}
			if (j == 30 * 11) {
				for (int k = 0; k < 11; k++) {
					msg->checksum.push_back(RightSoundbits[i]);
					i++;
				}
			}
			rawData.push_back(*msg);
		}

		// The checkSumValue and the sumDataValue arent equal which is weird!!!!
		//cout << " sum data " << rawData[0].sumDataValue() << endl;
		//cout << " check sum data " << rawData[0].checksumValue() << endl;

		for (int i = (SignalStartIndex + (LEADER_SIZE * 11) + (ID_SIZE * 11) +
			(MESSAGE_SIZE * (MESSAGE_DATA_SIZE + 1) * 11));
			i < (SignalStartIndex + (LEADER_SIZE * 11) + (ID_SIZE * 11) +
				(MESSAGE_SIZE * (MESSAGE_DATA_SIZE + 1) * 11) + 11); i++) {

			lastByte.push_back(RightSoundbits[i]);
		}

		for (int i = (SignalStartIndex + (LEADER_SIZE * 11) + (ID_SIZE * 11) +
			(MESSAGE_SIZE * (MESSAGE_DATA_SIZE + 1) * 11) + 11);
			i < (SignalStartIndex + (LEADER_SIZE * 11) + (ID_SIZE * 11) +
				(MESSAGE_SIZE * (MESSAGE_DATA_SIZE + 1) * 11) + 11 + (ENDBLOCK_SIZE * 11)); i++) {

			endBlock.push_back(RightSoundbits[i]);
		}
	}
};


void DecodeFSK(vector<__int16> Samples, vector<bool>& DigitalData , int OneSamples, int ZeroSamples) {
	__int16 oldValue = Samples[0];
	__int16 value;
	int i = 1;
	int j = 1;
	int count = 0;

	while (j < Samples.size()) {
		value = Samples[j];
		if (i == OneSamples && oldValue != value) {
			i = 0;
			DigitalData.push_back(1);

			count++;
		}
		if (i == ZeroSamples && oldValue != value) {
			i = 0;
			DigitalData.push_back(0);
			count++;
		}
		i++;
		j++;
		oldValue = value;
	}
	if (i == OneSamples) {
		DigitalData.push_back(1);
		count++;
	}
	if (i == ZeroSamples) {
		DigitalData.push_back(0);
		count++;
	}
	
}



int main() {

	// open the file for reading
	FILE* f = fopen("file_1.wav", "rb");
	 
	
	// check if the file is open
	if (!f) {
		cout << "fails to open the file" << endl;
		
	}
	else {

		////////////////////// read the header of the file
		wav_header_t header = readHeader(f);




		////////////////////// read and decode the main data

		// READ THE DATA


		__int16 sample;
		vector<__int16> RightSamples;
		vector<__int16> LeftSamples;
		/* the right boolean serves to add the sample
		   whether to the right Sound Samples or the left one 
		   since the signal is stereo (two channels)*/
		bool right = true; 
		while (!feof(f)) {

			fread(&sample, sizeof(__int16), 1, f);
			if (sample != 0) {
				if (right) {
					RightSamples.push_back(sample);
					right = false;
			
				}
				else {
					LeftSamples.push_back(sample);
					right = true;

				}
				
			}

		}
		//cout << "size of rightSamples " << RightSamples.size() << endl;
		//cout << "size of leftSamples " << LeftSamples.size() << endl;


		// DECODE THE DATA

		/*
		One bit is a rectangle signal of t = 320
		Zero bit is a rectangle signal of t = 640
		Giving that the sample rate is 44100 samples per second,
		it means 28 samples per 640 microseconds and 14 samples per 320 microseconds.
		*/

		int OneSamples = 320 * pow(10, -6) * header.sampleRate;
		int ZeroSamples = 640 * pow(10, -6) * header.sampleRate;

		vector <bool> RightSoundbits;
		DecodeFSK(RightSamples, RightSoundbits, OneSamples, ZeroSamples);
		cout << "the count of bits in right sound : " << RightSoundbits.size() << endl;

		vector <bool> LeftSoundbits;
		DecodeFSK(LeftSamples, LeftSoundbits, OneSamples, ZeroSamples);
		cout << "the count of bits in left sound : " << LeftSoundbits.size() << endl;


		// WRITE THE DATA IN A FILE AND A SUITABLE CONTAINER


		// write the data in a generated file


		ofstream binaryRightDatafile;
		binaryRightDatafile.open("binaryRightDatafile");
		// Since the signal starts with an infinit values of 0, 
		// we would like to track the real start index in the data
		int SignalStartIndex = 0; 
		// declares when we will start to get the data
		bool start = false;
		for (int i = 0; i < RightSoundbits.size(); i++) {
			if (start) {
				binaryRightDatafile << RightSoundbits[i] << endl;
			}
			if (RightSoundbits[i] == 1 && !start) {
				SignalStartIndex = i - 1;
				binaryRightDatafile << 0 << endl;
				binaryRightDatafile << 1 << endl;
				start = true;
			}

		}
		binaryRightDatafile.close();

		// write the data in a DataContainer


		DataContainer rightDataContainer;

		rightDataContainer.ReadToDataContainer(SignalStartIndex, RightSoundbits);

		///// CHECK THE ID : 
		cout << " The ID of this digital data is : ";
		for (int i = 0; i < ID_SIZE * 11; i++) {
			cout << rightDataContainer.ID[i];
		}




	}

	
	return 0;
}