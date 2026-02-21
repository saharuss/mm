/**
 * main_nsp.c — TI-Nspire CX entry point for Majora's Mask
 *
 * Architecture overview:
 *
 * On the N64, MM's boot sequence is:
 *   idle thread → Main() → creates graph thread → Graph_ThreadEntry()
 *   Graph_ThreadEntry loops: load overlay → GameState_Init → Graph_Update loop
 *   Graph_Update: GameState_GetInput → GameState_IncrementFrameCount →
 *                 Audio_Update → Graph_ExecuteAndDraw
 *   Graph_ExecuteAndDraw: GameState_Update (builds display lists) →
 *                         Graph_TaskSet00 (sends display list to RCP)
 *
 * On the Nspire, we:
 *   1. Initialize LCD + software renderer
 *   2. Call Graph_ThreadEntry directly (it handles its own loop)
 *   3. Intercept Graph_TaskSet00 to route display lists to our renderer
 *   4. Replace overlay loading with static function pointers
 */
#ifdef TARGET_NSP
#include <libndls.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * Forward declarations for the rendering backend (from sm64-nsp)
 * ============================================================ */

/* Nspire LCD + renderer */
extern void nsp_init(const char* name, bool fullscreen);
extern void nsp_swap_buffers_begin(void);
extern void nsp_swap_buffers_end(void);

/* Input */
extern void input_nsp_poll(void);
extern bool input_nsp_escape_pressed(void);

/* Config */
extern void configfile_load(const char* filename);
extern void configfile_save(const char* filename);

/* Timer */
#ifdef TARGET_NSP
extern void tmr_init(void);
extern uint32_t tmr_ms(void);
#else
static inline void tmr_init(void) {
}
static inline uint32_t tmr_ms(void) {
    return 0;
}
#endif

/* ============================================================
 * Globals that MM's code expects to exist
 * ============================================================ */
#include "nspire/platform/os_stubs.h"
#include "nspire/gfx/gbi_nsp.h"

/* Display list master — MM generates display lists into this */
/* These are defined as externs in MM code; we provide the storage */

/* Frame timing */
static uint32_t nsp_frame_count = 0;
static uint32_t nsp_last_fps_time = 0;
uint32_t nsp_current_fps = 0;

static void nsp_update_fps(void) {
    nsp_frame_count++;
    uint32_t now = tmr_ms();
    if (now - nsp_last_fps_time >= 1000) {
        nsp_current_fps = nsp_frame_count;
        nsp_frame_count = 0;
        nsp_last_fps_time = now;
    }
}

/* ============================================================
 * Display List Processing
 *
 * This is the heart of the port. When MM calls Graph_TaskSet00(),
 * instead of sending the display list to the N64 RCP, we walk it
 * and route each command to sm64-nsp's software renderer.
 * ============================================================ */

/* Segment table for segmented addressing */
static uintptr_t nsp_segments[16];

void nsp_set_segment(uint32_t seg, void* addr) {
    nsp_segments[seg & 0xF] = (uintptr_t)addr;
}

void* nsp_segmented_to_virtual(uint32_t addr) {
    uint32_t seg = (addr >> 24) & 0xF;
    uint32_t offset = addr & 0x00FFFFFF;
    if (nsp_segments[seg] == 0)
        return (void*)(uintptr_t)offset;
    return (void*)(nsp_segments[seg] + offset);
}

/**
 * Walk a display list and dispatch commands to the software renderer.
 *
 * This is called from our replacement Graph_TaskSet00.
 * The display list pointer comes from gGfxMasterDL->taskStart,
 * which is where Graph_ExecuteAndDraw builds the master display list.
 */
#define DL_STACK_SIZE 16
static Gfx* dl_stack[DL_STACK_SIZE];
static int dl_stack_top = 0;

static void dl_push(Gfx* dl) {
    if (dl_stack_top < DL_STACK_SIZE) {
        dl_stack[dl_stack_top++] = dl;
    }
}

static Gfx* dl_pop(void) {
    if (dl_stack_top > 0) {
        return dl_stack[--dl_stack_top];
    }
    return NULL;
}

void nsp_process_display_list(Gfx* dl) {
    if (!dl)
        return;

    dl_stack_top = 0;

    while (1) {
        uint32_t w0 = dl->w0;
        uint32_t w1 = dl->w1;
        uint8_t cmd = (w0 >> 24) & 0xFF;

        switch (cmd) {
            case G_ENDDL: {
                /* End of this display list — pop the stack */
                Gfx* parent = dl_pop();
                if (parent) {
                    dl = parent;
                    continue;
                }
                return; /* No more parent DLs, we're done */
            }

            case G_DL: {
                /* Call or branch to child display list */
                Gfx* child = (Gfx*)(uintptr_t)w1;
                if (child) {
                    bool is_branch = (w0 & 0x00FF0000) == 0x00010000;
                    if (!is_branch) {
                        /* Call: push return address */
                        dl_push(dl + 1);
                    }
                    dl = child;
                    continue;
                }
                break;
            }

            case G_MOVEMEM: {
                /* Handle segment setting */
                uint8_t index = (w0 >> 8) & 0xFF;
                uint8_t offset = (w0 >> 0) & 0xFF;
                /* Check if this is a segment command (MW_SEGMENT) */
                if (index == G_MW_SEGMENT) {
                    uint32_t seg_id = offset / 4;
                    nsp_set_segment(seg_id, (void*)(uintptr_t)w1);
                }
                break;
            }

            /* All graphics commands passed through to the software renderer.
             * The sm64-nsp gfx_frontend already handles all of these GBI
             * commands via its own display list interpreter.
             *
             * TODO: Route these to gfx_run_dl() once the frontend is
             * hooked up. For now, the game state machinery runs but
             * rendering is a no-op until the frontend glue is complete. */
            case G_VTX:
            case G_TRI1:
            case G_TRI2:
            case G_MTX:
            case G_POPMTX:
            case G_GEOMETRYMODE:
            case G_TEXTURE:
            case G_SETCOMBINE:
            case G_SETTIMG:
            case G_SETTILE:
            case G_SETTILESIZE:
            case G_LOADBLOCK:
            case G_LOADTILE:
            case G_LOADTLUT:
            case G_SETSCISSOR:
            case G_SETENVCOLOR:
            case G_SETPRIMCOLOR:
            case G_SETFOGCOLOR:
            case G_SETFILLCOLOR:
            case G_FILLRECT:
            case G_SETCIMG:
            case G_SETZIMG:
            case G_RDPSETOTHERMODE:
            case G_SETOTHERMODE_L:
            case G_SETOTHERMODE_H:
            case G_TEXRECT:
                /* These would go to the software renderer */
                break;

            case G_RDPPIPESYNC:
            case G_RDPTILESYNC:
            case G_RDPLOADSYNC:
            case G_RDPFULLSYNC:
            case G_SPNOOP:
            case G_NOOP:
                /* Sync/NOP — no-ops for software renderer */
                break;

            default:
                break;
        }

        dl++;
    }
}

/* ============================================================
 * ROM File-Based Asset Loading
 *
 * MM loads assets from the N64 ROM cartridge via DMA (PI interface).
 * On the Nspire, we read from a ROM file on the filesystem.
 * ============================================================ */

static FILE* rom_file = NULL;

/**
 * Open the ROM file for asset reading.
 * Must be called before any DMA operations.
 */
int nsp_rom_init(const char* rom_path) {
    rom_file = fopen(rom_path, "rb");
    return (rom_file != NULL) ? 0 : -1;
}

void nsp_rom_close(void) {
    if (rom_file) {
        fclose(rom_file);
        rom_file = NULL;
    }
}

/**
 * Read data from the ROM file.
 * Replaces N64 PI DMA transfers (osPiStartDma).
 *
 * @param rom_addr  Offset into the ROM (the "VROM" address)
 * @param dest      RAM destination buffer
 * @param size      Number of bytes to read
 * @return          0 on success, -1 on failure
 */
int nsp_rom_read(uint32_t rom_addr, void* dest, uint32_t size) {
    if (!rom_file)
        return -1;
    fseek(rom_file, rom_addr, SEEK_SET);
    size_t read = fread(dest, 1, size, rom_file);
    return (read == size) ? 0 : -1;
}

/* ============================================================
 * Static Overlay Linking
 *
 * On the N64, game state overlays and actor overlays are loaded
 * dynamically from ROM. On the Nspire, we statically link
 * everything and provide direct function pointers.
 * ============================================================ */

/* Forward declarations for game state init functions.
 * These are defined in MM's overlay source files. */
struct GameState;
typedef void (*GameStateFunc)(struct GameState*);

/* Game state overlay init functions (from MM source) */
extern void TitleSetup_Init(void* gameState);
extern void Select_Init(void* gameState);
extern void Title_Init(void* gameState);
extern void Opening_Init(void* gameState);
extern void FileSelect_Init(void* gameState);
extern void Play_Init(void* gameState);

/**
 * Replacement for Overlay_LoadGameState.
 * Instead of loading from ROM and relocating, we just ensure
 * the function pointers are already set (since we statically linked).
 */
void nsp_overlay_load_gamestate(void* overlayEntry) {
    /* No-op: everything is statically linked.
     * The game state table's init/destroy pointers already
     * point to the correct functions. */
    (void)overlayEntry;
}

/**
 * Replacement for Overlay_FreeGameState.
 */
void nsp_overlay_free_gamestate(void* overlayEntry) {
    (void)overlayEntry;
}

/* ============================================================
 * Nspire Main Entry Point
 *
 * Strategy: Rather than reimplementing the entire game loop,
 * we let MM's existing Graph_ThreadEntry run. We just need to:
 * 1. Initialize the Nspire hardware
 * 2. Provide stubs for all N64-specific functions it calls
 * 3. Intercept Graph_TaskSet00 to process display lists
 * 4. Call Graph_ThreadEntry directly
 * ============================================================ */

/* MM's entry point — we call this directly */
extern void Graph_ThreadEntry(void* arg);

/* Provided by MM code */
extern void SysCfb_Init(void);
extern void Fault_Init(void);

int main(void) {
    /* Initialize config */
    configfile_load("mm-nsp.cfg");

    /* Initialize Nspire LCD */
    nsp_init("Majora's Mask", false);
    tmr_init();

    /* Open ROM file for asset loading */
    if (nsp_rom_init("mm-us.z64") != 0) {
        /* ROM not found — can't run without it */
#ifdef TARGET_NSP
        /* Show error on Nspire screen */
        /* TODO: nio_printf error */
#endif
        return 1;
    }

    /*
     * Call MM's Graph_ThreadEntry directly.
     *
     * This function does everything:
     * - Allocates gfx pools
     * - Initializes GraphicsContext
     * - Loops through game states (title → file select → gameplay)
     * - Each frame calls Graph_Update → Graph_ExecuteAndDraw
     * - Will return when the game exits
     *
     * The key interception points are:
     * - Graph_TaskSet00: we replace this to process display lists
     *   through our software renderer instead of the N64 RCP
     * - PadMgr_GetInput: we replace this to read from Nspire keypad
     * - All audio functions: stubbed to no-ops
     * - Overlay loading: replaced with static linking
     */
    Graph_ThreadEntry(NULL);

    /* Cleanup */
    nsp_rom_close();
    configfile_save("mm-nsp.cfg");

#ifdef TARGET_NSP
    lcd_init(SCR_320x240_565); /* Reset LCD */
#endif

    return 0;
}
