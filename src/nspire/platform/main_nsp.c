/**
 * main_nsp.c — TI-Nspire CX entry point for Majora's Mask
 *
 * This replaces the N64's boot sequence (idle thread → main → graph thread)
 * with a single-threaded game loop on the Nspire.
 *
 * On the N64, the game runs as:
 *   1. Idle thread starts Main()
 *   2. Main() creates the Graph thread
 *   3. Graph thread creates game states and runs the game loop
 *   4. Each frame, Graph_TaskSet00 sends display lists to the RCP
 *
 * On the Nspire, we flatten this into:
 *   1. main() initializes the LCD and software renderer
 *   2. Game loop: poll input → update game state → process display list → blit
 */
#ifdef TARGET_NSP
#include <libndls.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "nspire/platform/os_stubs.h"
#include "nspire/gfx/gbi_nsp.h"
#include "nspire/configfile.h"
#include "nspire/fixed_pt.h"

/* Forward declarations for the rendering backend (from sm64-nsp) */
extern void gfx_init(void);
extern void gfx_start_frame(void);
extern void gfx_run_dl(Gfx* dl);
extern void gfx_end_frame(void);
extern void nsp_init(const char* name, bool fullscreen);
extern void nsp_swap_buffers_begin(void);
extern void nsp_swap_buffers_end(void);
extern int nsp_get_dimensions(uint32_t* w, uint32_t* h);

/* Forward declarations for input */
extern void input_nsp_poll(void);
extern bool input_nsp_escape_pressed(void);

/* Forward declarations for the game — these exist in MM's code */
extern void GameState_Init(void* gameState);
extern void GameState_Update(void* gameState);
extern void Graph_ThreadEntry(void* arg);

/* Timer utilities */
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
 * Display List Processing
 *
 * MM builds display lists (arrays of Gfx commands) each frame.
 * On the N64, these are sent to the RSP/RDP for hardware execution.
 * Here we walk the display list and dispatch each command to
 * the sm64-nsp software renderer.
 * ============================================================ */

/* Segment table — MM uses segmented addressing for display lists */
static uintptr_t segment_table[16];

void nsp_set_segment(uint32_t seg, void* addr) {
    segment_table[seg & 0xF] = (uintptr_t)addr;
}

void* nsp_segmented_to_virtual(uint32_t addr) {
    uint32_t seg = (addr >> 24) & 0xF;
    uint32_t offset = addr & 0x00FFFFFF;
    return (void*)(segment_table[seg] + offset);
}

/**
 * Process a single display list recursively.
 * This is the core of the Nspire port — it interprets the Gfx commands
 * that MM generates and routes them to the software renderer.
 */
void nsp_process_display_list(Gfx* dl) {
    if (!dl)
        return;

    /* The display list is an array of 64-bit commands.
     * We iterate until we hit G_ENDDL. */
    while (1) {
        uint32_t w0 = dl->w0;
        uint32_t w1 = dl->w1;
        uint8_t cmd = (w0 >> 24) & 0xFF;

        switch (cmd) {
            case G_ENDDL:
                return;

            case G_DL: {
                /* Call or branch to a child display list */
                Gfx* child = (Gfx*)(uintptr_t)w1;
                if (child) {
                    nsp_process_display_list(child);
                }
                /* If this is a branch (not call), return after */
                if ((w0 & 0x00FF0000) == 0x00010000)
                    return;
                break;
            }

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
            case G_MOVEMEM:
                /* All of these are passed to the software renderer's
                 * display list interpreter (gfx_frontend.c from sm64-nsp).
                 * The renderer already knows how to handle all these GBI commands.
                 *
                 * TODO: Hook up gfx_run_dl() to process these commands.
                 *       For now, we just skip them. */
                break;

            case G_RDPPIPESYNC:
            case G_RDPTILESYNC:
            case G_RDPLOADSYNC:
            case G_RDPFULLSYNC:
            case G_SPNOOP:
            case G_NOOP:
                /* Sync/NOP commands — no-ops for software renderer */
                break;

            default:
                /* Unknown command — skip */
                break;
        }

        dl++;
    }
}

/* ============================================================
 * Frame Timing
 * ============================================================ */
static uint32_t frame_count = 0;
static uint32_t last_fps_time = 0;
static uint32_t current_fps = 0;

static void update_fps(void) {
    frame_count++;
    uint32_t now = tmr_ms();
    if (now - last_fps_time >= 1000) {
        current_fps = frame_count;
        frame_count = 0;
        last_fps_time = now;
    }
}

/* ============================================================
 * Nspire Main Entry Point
 * ============================================================ */

/**
 * The main game loop for the Nspire port.
 *
 * This replaces the N64's multi-threaded architecture with a simple
 * single-threaded loop:
 *
 *   1. Initialize LCD + software renderer
 *   2. Load config
 *   3. Loop:
 *      a. Poll Nspire keypad → N64 controller state
 *      b. Run one frame of game logic
 *      c. Process the generated display list
 *      d. Blit framebuffer to LCD
 *      e. Check for ESC (quit)
 */
int main(void) {
    /* Initialize config */
    configfile_load("mm-nsp.cfg");

    /* Initialize Nspire LCD */
    nsp_init("Majora's Mask", false);
    tmr_init();

    /* Initialize software renderer */
    gfx_init();

    /* Initialize game
     * On the N64, Main() creates threads and Graph_ThreadEntry
     * runs the game loop. Here we'd need to initialize game state
     * directly. This is the part that requires the most work —
     * we need to call the game's initialization without going
     * through the N64 threading system.
     *
     * TODO: This is where the bulk of remaining integration work lives.
     * Game state initialization needs to be called directly,
     * and the game loop needs to be driven from here instead of
     * from Graph_ThreadEntry.
     */

    /* Main game loop */
    bool running = true;
    uint32_t frameskip_counter = 0;

    while (running) {
        /* Poll input */
        input_nsp_poll();

        /* Check quit */
        if (input_nsp_escape_pressed()) {
            running = false;
            break;
        }

        /* Frameskip — only render every (configFrameskip + 1) frames */
        frameskip_counter++;
        bool should_render = (frameskip_counter > configFrameskip);
        if (should_render) {
            frameskip_counter = 0;
        }

        /* TODO: Run one frame of game logic here
         * This would call the game state update function,
         * which builds up a display list.
         */

        /* Render frame */
        if (should_render) {
            gfx_start_frame();

            /* TODO: Process the display list generated by the game
             * nsp_process_display_list(game_display_list);
             */

            gfx_end_frame();

            /* Blit to LCD */
            nsp_swap_buffers_begin();
            nsp_swap_buffers_end();
        }

        update_fps();
    }

    /* Cleanup */
    configfile_save("mm-nsp.cfg");

#ifdef TARGET_NSP
    lcd_init(SCR_320x240_565); /* Reset LCD */
#endif

    return 0;
}
