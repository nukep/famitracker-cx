#ifndef _FDSSOUND_H_
#define _FDSSOUND_H_

#if defined(__i386__) || defined(_M_IX86)
#define FDSCALL __fastcall
#else
#define FDSCALL
#endif

void FDSCALL FDSSoundReset(void);
uint8 FDSCALL FDSSoundRead(uint16 address);
void FDSCALL FDSSoundWrite(uint16 address, uint8 value);
int32 FDSCALL FDSSoundRender(void);
void FDSCALL FDSSoundVolume(unsigned int volume);
void FDSSoundInstall3(void);

#endif /* _FDSSOUND_H_ */
