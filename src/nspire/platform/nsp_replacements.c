/**
 * nsp_replacements.c — Function replacements for N64→Nspire port
 *
 * These replace N64-specific functions that MM calls throughout its code.
 * They're compiled into the Nspire build to override the original
 * implementations.
 *
 * Key replacements:
 * - Graph_TaskSet00: intercept display lists → software renderer
 * - PadMgr_GetInput: read from Nspire keypad
 * - Overlay_LoadGameState/Free: no-op (static linking)
 * - DMA: file-based ROM reading
 * - SysCfb: fixed framebuffer allocation
 * - Fault: simplified error handling
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef TARGET_NSP
#include <libndls.h>
#endif

#include "nspire/platform/os_stubs.h"
#include "nspire/gfx/gbi_nsp.h"

/* ============================================================
 * External declarations
 * ============================================================ */

/* From main_nsp.c */
extern void nsp_process_display_list(Gfx* dl);
extern int nsp_rom_read(uint32_t rom_addr, void* dest, uint32_t size);

/* From sm64-nsp renderer */
extern void nsp_swap_buffers_begin(void);
extern void nsp_swap_buffers_end(void);

/* From input_nsp.c */
extern void input_nsp_poll(void);

/* Config */
extern bool configAffineMode;
extern unsigned int configFrameskip;

/* ============================================================
 * Graph_TaskSet00 Replacement
 *
 * On N64: constructs an OSTask and sends display list to RCP scheduler
 * On Nspire: processes the display list through our software renderer
 * and blits the result to the LCD
 * ============================================================ */

/* MM's graphics master display list structure */
/* We need the actual type from MM, but for now use the pointer */
extern void* gGfxMasterDL;

static uint32_t frameskip_counter = 0;

/**
 * Replacement for Graph_TaskSet00.
 * Instead of sending the display list to the N64 RCP,
 * we process it through our software renderer.
 *
 * This is called at the end of each frame by Graph_ExecuteAndDraw.
 */
void Graph_TaskSet00_Nsp(void* gfxCtx, void* gameState) {
    (void)gameState;

    /* Frameskip — only render every N frames */
    frameskip_counter++;
    if (frameskip_counter <= configFrameskip) {
        return;
    }
    frameskip_counter = 0;

    /* Get the master display list pointer.
     * In MM, Graph_ExecuteAndDraw builds the master DL at
     * gGfxMasterDL->taskStart, which chains to the per-buffer
     * display lists (POLY_OPA, POLY_XLU, OVERLAY, etc.).
     *
     * The task->data_ptr field points to gGfxMasterDL,
     * which is the root of the display list tree.
     */
    if (gGfxMasterDL != NULL) {
        /* Process the display list through our renderer */
        nsp_process_display_list((Gfx*)gGfxMasterDL);
    }

    /* Blit to LCD */
    nsp_swap_buffers_begin();
    nsp_swap_buffers_end();
}

/* ============================================================
 * PadMgr Replacement
 *
 * PadMgr_GetInput is called every frame to read controller state.
 * We replace it with Nspire keypad reading.
 * ============================================================ */

/* N64 Input structure (matches MM's Input type) */
typedef struct {
    /* 0x00 */ u16 cur_button;
    /* 0x02 */ s8 cur_x;
    /* 0x03 */ s8 cur_y;
    /* 0x04 */ u8 errno_;
    /* 0x06 */ u16 prev_button;
    /* 0x08 */ s8 prev_x;
    /* 0x09 */ s8 prev_y;
    /* 0x0A */ u8 prev_errno;
    /* 0x0C */ u16 press_button; /* buttons pressed this frame */
    /* 0x0E */ s8 press_x;
    /* 0x0F */ s8 press_y;
    /* 0x10 */ u16 rel_button; /* buttons released this frame */
    /* 0x12 */ s8 rel_x;
    /* 0x13 */ s8 rel_y;
} InputCompat;

/* From input_nsp.c */
typedef struct {
    u16 button;
    s8 stick_x;
    s8 stick_y;
    u8 errnum;
} OSContPad;
extern OSContPad* input_nsp_get_pad(void);

/**
 * Replacement for PadMgr_GetInput.
 * Reads the Nspire keypad and fills MM's Input structures.
 *
 * @param input   Array of 4 Input structures (one per controller port)
 * @param lockInput  If true, clear all input (used during certain transitions)
 */
void PadMgr_GetInput_Nsp(void* input_ptr, s32 lockInput) {
    InputCompat* input = (InputCompat*)input_ptr;

    /* Poll the Nspire keypad */
    input_nsp_poll();

    if (lockInput) {
        memset(input, 0, sizeof(InputCompat) * 4);
        return;
    }

    /* Controller 1 */
    OSContPad* pad = input_nsp_get_pad();

    /* Save previous state */
    input[0].prev_button = input[0].cur_button;
    input[0].prev_x = input[0].cur_x;
    input[0].prev_y = input[0].cur_y;

    /* Read current state */
    input[0].cur_button = pad->button;
    input[0].cur_x = pad->stick_x;
    input[0].cur_y = pad->stick_y;
    input[0].errno_ = 0;

    /* Calculate press/release */
    input[0].press_button = (input[0].cur_button ^ input[0].prev_button) & input[0].cur_button;
    input[0].rel_button = (input[0].cur_button ^ input[0].prev_button) & input[0].prev_button;
    input[0].press_x = input[0].cur_x - input[0].prev_x;
    input[0].press_y = input[0].cur_y - input[0].prev_y;

    /* Clear other controllers */
    memset(&input[1], 0, sizeof(InputCompat) * 3);
}

/* ============================================================
 * DMA / ROM Loading Replacements
 *
 * MM uses DMA (PI interface) to load assets from the ROM cartridge.
 * We replace these with file reads from mm-us.z64.
 * ============================================================ */

/**
 * Replacement for DmaMgr_SendRequest / DmaRequest.
 * Reads data directly from the ROM file.
 */
s32 DmaMgr_RequestSync(void* ram, u32 vrom, u32 size) {
    return nsp_rom_read(vrom, ram, size);
}

s32 DmaMgr_RequestAsync(void* request, void* ram, u32 vrom, u32 size, u32 unk, void* queue, void* msg) {
    (void)request;
    (void)unk;
    (void)queue;
    (void)msg;
    return nsp_rom_read(vrom, ram, size);
}

/* DmaMgr_SendRequestImpl — the core DMA function in MM */
s32 DmaMgr_SendRequestImpl(void* request, void* ram, u32 vrom, u32 size) {
    return nsp_rom_read(vrom, ram, size);
}

/* ============================================================
 * Overlay Loading Replacements (Static Linking)
 *
 * On the N64, overlays (actors, game states, effects) are loaded
 * dynamically from ROM and relocated to RAM. On Nspire, everything
 * is statically linked, so these become no-ops.
 * ============================================================ */

/**
 * Replacement for Overlay_AllocateAndLoad.
 * Since everything is statically linked, return NULL to indicate
 * "already loaded" (the vram pointers are valid directly).
 */
void* Overlay_AllocateAndLoad(u32 vromStart, u32 vromEnd, void* vramStart, void* vramEnd) {
    (void)vromStart;
    (void)vromEnd;
    (void)vramStart;
    (void)vramEnd;
    /* Return NULL — signals that no relocation is needed
     * because the code is already at the correct address
     * (statically linked). */
    return NULL;
}

/* ============================================================
 * SysCfb — Framebuffer allocation
 * ============================================================ */

/* MM's framebuffer globals */
static uint16_t gWorkBufferStorage[SCREEN_WIDTH * SCREEN_HEIGHT] __attribute__((aligned(64)));
static uint16_t gZBufferStorage[SCREEN_WIDTH * SCREEN_HEIGHT] __attribute__((aligned(64)));

void* gWorkBuffer = gWorkBufferStorage;
void* gWorkBufferLoRes = NULL;
void* gZBufferLoRes = NULL;

void SysCfb_Init(void) {
    /* Framebuffers are statically allocated above */
    gWorkBufferLoRes = gWorkBufferStorage;
}

/* ============================================================
 * Fault System Stubs
 * ============================================================ */

typedef struct {
    int dummy;
} FaultClient;
typedef struct {
    int dummy;
} FaultAddrConvClient;

void Fault_Init(void) {
}
void Fault_AddClient(void* client, void* callback, void* arg1, void* arg2) {
    (void)client;
    (void)callback;
    (void)arg1;
    (void)arg2;
}
void Fault_RemoveClient(void* client) {
    (void)client;
}
void Fault_AddAddrConvClient(void* client, void* callback, void* param) {
    (void)client;
    (void)callback;
    (void)param;
}
void Fault_RemoveAddrConvClient(void* client) {
    (void)client;
}
void Fault_SetFrameBuffer(void* fb, s32 w, s32 h) {
    (void)fb;
    (void)w;
    (void)h;
}
void Fault_AddHungupAndCrash(const char* file, s32 line) {
    (void)file;
    (void)line;
    /* On Nspire: just exit */
#ifdef TARGET_NSP
    exit(1);
#endif
}
void Fault_AddHungupAndCrashImpl(const char* str1, const char* str2) {
    (void)str1;
    (void)str2;
#ifdef TARGET_NSP
    exit(1);
#endif
}

/* ============================================================
 * IRQ Manager Stubs
 * ============================================================ */
typedef struct {
    int dummy;
} IrqMgrClient;
typedef struct {
    OSMesgQueue cmdQueue;
    OSMesg cmdBuf[8];
} SchedulerCompat;

SchedulerCompat gScheduler;

void IrqMgr_Init(void) {
}
void IrqMgr_AddClient(void* mgr, void* client, void* queue) {
    (void)mgr;
    (void)client;
    (void)queue;
}
void IrqMgr_RemoveClient(void* mgr, void* client) {
    (void)mgr;
    (void)client;
}

void Sched_Init(void* sched, void* stack, s32 pri, s32 viMode, s32 numFields, void* irqMgr) {
    (void)sched;
    (void)stack;
    (void)pri;
    (void)viMode;
    (void)numFields;
    (void)irqMgr;
    /* Initialize the scheduler's message queue */
    osCreateMesgQueue(&gScheduler.cmdQueue, gScheduler.cmdBuf, 8);
}

void Sched_SendGfxCancelMsg(void* sched) {
    (void)sched;
}
void Sched_SendNotifyMsg(void* sched) {
    (void)sched;
}

/* ============================================================
 * Misc Stubs
 * ============================================================ */

/* NMI (Non-Maskable Interrupt) */
void Nmi_Init(void) {
}
void Nmi_SetPrenmiStart(void) {
}

/* System checks */
void Check_RegionIsSupported(void) {
}
void Check_ExpansionPak(void) {
}

/* Ucode */
void* SysUcode_GetUCodeBoot(void) {
    return NULL;
}
u32 SysUcode_GetUCodeBootSize(void) {
    return 0;
}
void* SysUcode_GetUCode(void) {
    return NULL;
}
void* SysUcode_GetUCodeData(void) {
    return NULL;
}

/* CIC */
void CIC6105_AddRomInfoFaultPage(void) {
}

/* Stack check */
typedef struct {
    int dummy;
} StackEntry;
void StackCheck_Init(void* entry, void* start, void* end, s32 unk, s32 margin, const char* name) {
    (void)entry;
    (void)start;
    (void)end;
    (void)unk;
    (void)margin;
    (void)name;
}

/* Registers system */
void Regs_Init(void) {
}

/* Debug */
void _dbg_hungup(const char* file, s32 line) {
    (void)file;
    (void)line;
}
void osSyncPrintf(const char* fmt, ...) {
    (void)fmt; /* No debug output on Nspire */
}
void Debug_DrawText(void* gfxCtx) {
    (void)gfxCtx;
}

/* Speed meter */
typedef struct {
    int dummy;
} SpeedMeter;
void SpeedMeter_Init(void* sm) {
    (void)sm;
}
void SpeedMeter_Destroy(void* sm) {
    (void)sm;
}
void SpeedMeter_DrawTimeEntries(void* sm, void* gfxCtx) {
    (void)sm;
    (void)gfxCtx;
}
void SpeedMeter_DrawAllocEntries(void* sm, void* gfxCtx, void* gs) {
    (void)sm;
    (void)gfxCtx;
    (void)gs;
}

/* Vis effects */
typedef struct {
    int dummy;
} VisCvg;
typedef struct {
    int dummy;
} VisZBuf;
typedef struct {
    int dummy;
} VisMono;
typedef struct {
    int dummy;
} ViMode;

void VisCvg_Init(void* v) {
    (void)v;
}
void VisCvg_Destroy(void* v) {
    (void)v;
}
void VisCvg_Draw(void* v, void** gfx) {
    (void)v;
    (void)gfx;
}

void VisZBuf_Init(void* v) {
    (void)v;
}
void VisZBuf_Destroy(void* v) {
    (void)v;
}
void VisZBuf_Draw(void* v, void** gfx, void* zb) {
    (void)v;
    (void)gfx;
    (void)zb;
}

void VisMono_Init(void* v) {
    (void)v;
}
void VisMono_Destroy(void* v) {
    (void)v;
}
void VisMono_Draw(void* v, void** gfx) {
    (void)v;
    (void)gfx;
}

void ViMode_Init(void* v) {
    (void)v;
}
void ViMode_Destroy(void* v) {
    (void)v;
}

/* Rumble */
void Rumble_Init(void) {
}
void Rumble_Destroy(void) {
}

/* Timer */
void tmr_init(void) {
#ifdef TARGET_NSP
    /* Initialize Ndless timer */
#endif
}

uint32_t tmr_ms(void) {
#ifdef TARGET_NSP
    /* Return ms since timer init */
    return 0; /* TODO: implement with Ndless timer */
#else
    return 0;
#endif
}

/* Malloc wrappers */
void GetFreeArena(void* maxFree, void* bytesFree, void* bytesAllocated) {
    *(uint32_t*)maxFree = 32 * 1024 * 1024; /* Report 32MB free */
    *(uint32_t*)bytesFree = 32 * 1024 * 1024;
    *(uint32_t*)bytesAllocated = 0;
}

/* System heap */
void SystemHeap_Init(void* start, uint32_t size) {
    (void)start;
    (void)size;
    /* On Nspire, we use standard malloc which uses the system heap */
}

/* func_800809F4 — unknown function, stub */
void func_800809F4(u32 vromStart) {
    (void)vromStart;
}
