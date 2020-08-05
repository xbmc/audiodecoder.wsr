
#ifndef __AUDIO_H__
#define __AUDIO_H__

extern void ws_audio_init(void);
extern void ws_audio_reset(void);
extern void ws_audio_done(void);
extern void ws_audio_update(short *buffer, int length);
extern void ws_audio_port_write(BYTE port,BYTE value);
extern BYTE ws_audio_port_read(BYTE port);
extern void ws_audio_process(void);
extern void ws_audio_sounddma(void);
extern int WaveAdrs;

#endif
