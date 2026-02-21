/**
 * os_stubs.h — N64 libultra OS function stubs for TI-Nspire CX
 *
 * Replaces the N64's multi-threaded RTOS with single-threaded stubs.
 * The N64 uses cooperative multithreading via osCreateThread/osStartThread,
 * but the Nspire runs everything in a single thread with a simple game loop.
 */
#ifndef OS_STUBS_H
#define OS_STUBS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================
 * N64 OS Type Definitions
 * Replace the actual libultra types with minimal compatible defs
 * ============================================================ */

typedef int32_t s32;
typedef uint32_t u32;
typedef int16_t s16;
typedef uint16_t u16;
typedef int8_t s8;
typedef uint8_t u8;
typedef float f32;
typedef double f64;
typedef uintptr_t uintptr_t;

/* OS Message Queue — used for inter-thread communication on N64.
 * On Nspire, we just store the last message. */
typedef void* OSMesg;

typedef struct {
    OSMesg* msg;
    s32 msgCount;
    s32 validCount;
    s32 first;
} OSMesgQueue;

/* OS Thread — completely stubbed on Nspire */
typedef struct {
    s32 id;
    s32 priority;
    void (*entry)(void*);
    void* arg;
} OSThread;

/* OS Timer */
typedef struct {
    u32 interval;
    u32 value;
} OSTimer;

/* OS Task — RCP task descriptor (stubbed) */
typedef struct {
    u32 type;
    u32 flags;
    void* ucode_boot;
    u32 ucode_boot_size;
    void* ucode;
    u32 ucode_size;
    void* ucode_data;
    u32 ucode_data_size;
    void* dram_stack;
    u32 dram_stack_size;
    void* output_buff;
    void* output_buff_size;
    void* data_ptr;
    u32 data_size;
    void* yield_data_ptr;
    u32 yield_data_size;
} OSTask_t;

typedef union {
    OSTask_t t;
} OSTask;

/* OS Mesg constants */
#define OS_MESG_NOBLOCK 0
#define OS_MESG_BLOCK 1

/* OS Event types */
#define OS_EVENT_SI 0
#define OS_EVENT_SP 1
#define OS_EVENT_DP 2
#define OS_EVENT_AI 3
#define OS_EVENT_VI 4
#define OS_EVENT_PI 5
#define OS_EVENT_PRENMI 6
#define OS_NUM_EVENTS 7

/* OS Priority levels (used by threads) */
#define OS_PRIORITY_IDLE 0
#define OS_PRIORITY_APPMAX 127

/* OS Scheduler message types */
#define OS_SC_PRE_NMI_MSG 1
#define OS_SC_NMI_MSG 2

/* ============================================================
 * Stubbed OS Functions
 * ============================================================ */

/* Thread functions — no-ops on single-threaded Nspire */
static inline void osCreateThread(OSThread* thread, s32 id, void (*entry)(void*), void* arg, void* sp, s32 pri) {
    thread->id = id;
    thread->priority = pri;
    thread->entry = entry;
    thread->arg = arg;
}

static inline void osStartThread(OSThread* thread) {
    /* In single-threaded mode, we just call the entry directly
     * when appropriate — not here */
    (void)thread;
}

static inline void osDestroyThread(OSThread* thread) {
    (void)thread;
}

static inline void osSetThreadPri(OSThread* thread, s32 pri) {
    if (thread)
        thread->priority = pri;
}

/* Message queue functions — simplified single-message queue */
static inline void osCreateMesgQueue(OSMesgQueue* mq, OSMesg* msg, s32 count) {
    mq->msg = msg;
    mq->msgCount = count;
    mq->validCount = 0;
    mq->first = 0;
}

static inline s32 osSendMesg(OSMesgQueue* mq, OSMesg msg, s32 flag) {
    if (mq->validCount < mq->msgCount) {
        s32 idx = (mq->first + mq->validCount) % mq->msgCount;
        mq->msg[idx] = msg;
        mq->validCount++;
        return 0;
    }
    return -1;
}

static inline s32 osRecvMesg(OSMesgQueue* mq, OSMesg* msg, s32 flag) {
    if (mq->validCount > 0) {
        if (msg)
            *msg = mq->msg[mq->first];
        mq->first = (mq->first + 1) % mq->msgCount;
        mq->validCount--;
        return 0;
    }
    if (flag == OS_MESG_BLOCK) {
        /* On N64 this would block until a message arrives.
         * On Nspire, we return immediately — caller must handle. */
    }
    return -1;
}

static inline s32 osJamMesg(OSMesgQueue* mq, OSMesg msg, s32 flag) {
    return osSendMesg(mq, msg, flag);
}

/* Event functions */
static inline void osSetEventMesg(s32 event, OSMesgQueue* mq, OSMesg msg) {
    (void)event;
    (void)mq;
    (void)msg;
}

/* Timer functions */
static inline s32 osSetTimer(OSTimer* timer, u32 countdown, u32 interval, OSMesgQueue* mq, OSMesg msg) {
    (void)timer;
    (void)countdown;
    (void)interval;
    (void)mq;
    (void)msg;
    return 0;
}

static inline s32 osStopTimer(OSTimer* timer) {
    (void)timer;
    return 0;
}

/* Virtual <-> Physical address stubs (no TLB on Nspire) */
static inline void* osVirtualToPhysical(void* addr) {
    return addr;
}
static inline void* osPhysicalToVirtual(void* addr) {
    return addr;
}

/* Cache operations — no-ops */
static inline void osInvalDCache(void* addr, s32 len) {
    (void)addr;
    (void)len;
}
static inline void osInvalICache(void* addr, s32 len) {
    (void)addr;
    (void)len;
}
static inline void osWritebackDCache(void* addr, s32 len) {
    (void)addr;
    (void)len;
}
static inline void osWritebackDCacheAll(void) {
}

/* VI (Video Interface) stubs */
static inline void osViSetMode(void* mode) {
    (void)mode;
}
static inline void osViSetSpecialFeatures(u32 feat) {
    (void)feat;
}
static inline void osViBlack(u32 black) {
    (void)black;
}
static inline void osViSwapBuffer(void* fb) {
    (void)fb;
}
static inline void* osViGetNextFramebuffer(void) {
    return NULL;
}
static inline void* osViGetCurrentFramebuffer(void) {
    return NULL;
}

/* Controller/SI stubs */
static inline s32 osContInit(OSMesgQueue* mq, u8* pattern, void* status) {
    (void)mq;
    (void)status;
    if (pattern)
        *pattern = 1; /* Report 1 controller */
    return 0;
}

static inline s32 osContStartReadData(OSMesgQueue* mq) {
    (void)mq;
    return 0;
}
static inline void osContGetReadData(void* data) {
    (void)data;
}

/* PI (Peripheral Interface) DMA — for ROM reads */
static inline s32 osPiStartDma(void* ioMesg, s32 priority, s32 direction, u32 devAddr, void* dramAddr, u32 size,
                               OSMesgQueue* mq) {
    /* In a real port, this would read from the ROM file.
     * For now, just memset to 0 */
    memset(dramAddr, 0, size);
    if (mq)
        osSendMesg(mq, NULL, OS_MESG_NOBLOCK);
    return 0;
}

/* Memory allocation */
static inline void* osGetMemSize(void) {
    return (void*)0x04000000; /* 64MB */
}

/* Misc */
static inline u32 osGetCount(void) {
    /* Return a monotonic counter — needs platform impl */
    static u32 counter = 0;
    return counter++;
}

static inline u64 osGetTime(void) {
    return (u64)osGetCount();
}

#endif /* OS_STUBS_H */
