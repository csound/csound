#include "KeyboardMapping.hpp"
#include <stdio.h>

KeyboardMapping::KeyboardMapping(CSOUND *csound, const char *mapFileName)
{

	for(int i = 0; i < 128; i++) {

		char * name = (char *)csound->Calloc(csound, 9);

		sprintf(name, "Bank %d", i + 1);

		banks.push_back(new Bank(csound, name));

	}

	currentChannel = 0;
	previousChannel = 0;


	for(int i = 0; i < 16; i++) {
		currentBank[i] = 0;
		previousBank[i] = -1;
	}
}

KeyboardMapping::~KeyboardMapping()
{
	for(unsigned int i = 0; i < banks.size(); i++) {
		delete banks[i];
	}
}

int KeyboardMapping::getCurrentChannel() {
	return currentChannel;
}

int KeyboardMapping::getCurrentBank() {
	return currentBank[currentChannel];
}

int KeyboardMapping::getPreviousBank() {
	return previousBank[currentChannel];
}

int KeyboardMapping::getCurrentProgram() {
	return banks[getCurrentBank()]->currentProgram;
}

int KeyboardMapping::getPreviousProgram() {
	return banks[getCurrentBank()]->previousProgram;
}


void KeyboardMapping::setCurrentChannel(int index) {
	currentChannel = index;
}

void KeyboardMapping::setCurrentBank(int index) {
	currentBank[currentChannel] = index;
}

void KeyboardMapping::setPreviousBank(int index) {
	previousBank[currentChannel] = index;
}

void KeyboardMapping::setCurrentProgram(int index) {
	banks[currentBank[currentChannel]]->currentProgram = index;
}

void KeyboardMapping::setPreviousProgram(int index) {
	banks[currentBank[currentChannel]]->previousProgram = index;
}


int KeyboardMapping::getCurrentBankMIDINumber() {
	return banks[getCurrentBank()]->bankNum;
}
