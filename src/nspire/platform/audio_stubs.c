/**
 * audio_stubs.c — Stub the entire audio system for TI-Nspire CX
 *
 * The Nspire has no speaker, so all audio functions are no-ops.
 * These stubs match the function signatures in MM's audio system.
 */
#include <stdint.h>
#include <stdbool.h>

typedef int32_t s32;
typedef uint32_t u32;
typedef int16_t s16;
typedef uint16_t u16;
typedef uint8_t u8;
typedef float f32;

/* AudioMgr — the main audio manager thread on N64 */
void AudioMgr_Init(void* audioMgr, void* stack, s32 pri, s32 id, void* sched, void* irqMgr) {
    (void)audioMgr;
    (void)stack;
    (void)pri;
    (void)id;
    (void)sched;
    (void)irqMgr;
}

void AudioMgr_Unlock(void* audioMgr) {
    (void)audioMgr;
}

/* Audio playback functions */
void Audio_PlaySfx(u16 sfxId) {
    (void)sfxId;
}
void Audio_PlaySfxGeneral(u16 sfxId, void* pos, u8 p2, void* p3, void* p4, void* p5) {
    (void)sfxId;
    (void)pos;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
}
void Audio_StopSfxById(u32 sfxId) {
    (void)sfxId;
}
void Audio_StopSfxByPos(void* pos) {
    (void)pos;
}
void Audio_StopSfxByPosAndId(void* pos, u16 sfxId) {
    (void)pos;
    (void)sfxId;
}

/* BGM (Background Music) */
void Audio_PlaySequenceBySeqId(u32 seqId) {
    (void)seqId;
}
void Audio_QueueSeqCmd(u32 cmd) {
    (void)cmd;
}
s32 Audio_IsSeqCmdNotQueued(u32 cmd, u32 p) {
    (void)cmd;
    (void)p;
    return 1;
}

/* Audio processing — called every frame */
void Audio_Update(void) {
}
void Audio_ResetForPool(void) {
}
void Audio_PreNMI(void) {
}

/* Volume control */
void Audio_SetSfxBanksMute(u16 muteMask) {
    (void)muteMask;
}
void Audio_SetBaseFilter(u8 filter) {
    (void)filter;
}
void Audio_ClearBGMMute(u16 channelIdx) {
    (void)channelIdx;
}
void Audio_PlayFanfare(u16 seqId) {
    (void)seqId;
}
void Audio_SetCutsceneFlag(s32 flag) {
    (void)flag;
}

/* Ocarina (Nspire can't exactly play one...) */
void Audio_OcaSetInstrument(u8 inst) {
    (void)inst;
}
u8 Audio_OcaGetRecordingState(void) {
    return 0;
}
void Audio_OcaSetRecordingState(u8 state) {
    (void)state;
}
