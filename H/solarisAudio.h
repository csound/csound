#ifndef solarisAudio
#define solarisAudio
#include <sys/audioio.h>
audio_info_t* audioGetDevInfo(int fd);
void audioSetDevInfo(int fd, audio_info_t* ai);
int audioGetPlayDevice(char *devName);
int audioGetRecordDevice(char *devName);
audio_prinfo_t audioGetPlayInfo(int fd);
audio_prinfo_t audioGetRecordInfo(int fd);
void audioSetPlayInfo(int fd, uint_t sampleRate, uint_t nOfChannels, uint_t precision, uint_t bufferSize);
void audioSetRecordInfo(int fd, uint_t sampleRate, uint_t nOfChannels, uint_t precision, uint_t bufferSize);
void audioPausePlay(int fd);
void audioResumePlay(int fd);
void audioPauseRecord(int fd);
void audioResumeRecord(int fd);
void audioPrintPlayInfo(int fd);
void audioPrintRecordInfo(int fd);
void audioPrintDevInfo(int fd);
void audioDie(int fd, char *message);
#endif
