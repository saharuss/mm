/**
 * gbi_nsp.h — GBI Display List Interceptor for TI-Nspire CX
 *
 * On the N64, GBI macros generate display list commands that are sent
 * to the RSP (Reality Signal Processor). On the Nspire, we intercept
 * these commands and route them directly to sm64-nsp's software renderer.
 *
 * This header replaces <PR/gbi.h> and translates GBI macros into
 * direct calls to the software rendering backend.
 */
#ifndef GBI_NSP_H
#define GBI_NSP_H

#include <stdint.h>

/* ============================================================
 * Gfx (Display List) type
 * On N64, each Gfx is a 64-bit command word.
 * We keep the same structure for source compatibility.
 * ============================================================ */
typedef struct {
    uint32_t w0;
    uint32_t w1;
} Gfx;

/* Display list pointer type */
typedef Gfx* Gfx_p;

/* Vertex type — matches N64 Vtx format */
typedef struct {
    int16_t ob[3]; /* position */
    uint16_t flag;
    int16_t tc[2]; /* texture coords */
    uint8_t cn[4]; /* color/normal */
} Vtx_t;

typedef union {
    Vtx_t v;
    long long int force_structure_alignment;
} Vtx;

/* Matrix type */
typedef int32_t Mtx_t[4][4];
typedef union {
    Mtx_t m;
    struct {
        uint16_t intPart[4][4];
        uint16_t fracPart[4][4];
    };
    long long int force_structure_alignment;
} Mtx;

/* Light type */
typedef struct {
    uint8_t col[3];
    int8_t pad1;
    uint8_t colc[3];
    int8_t pad2;
    int8_t dir[3];
    int8_t pad3;
} Light_t;

typedef struct {
    uint8_t col[3];
    int8_t pad1;
    uint8_t colc[3];
    int8_t pad2;
} Ambient_t;

typedef struct {
    Ambient_t a;
    Light_t l[7];
} Lights7;

typedef struct {
    Ambient_t a;
    Light_t l[1];
} Lights1;

/* Viewport */
typedef struct {
    int16_t vscale[4];
    int16_t vtrans[4];
} Vp_t;

typedef union {
    Vp_t vp;
    long long int force_structure_alignment;
} Vp;

/* Hilite (specular highlight) */
typedef struct {
    int32_t x1, y1, x2, y2;
} Hilite_t;

typedef union {
    Hilite_t h;
} Hilite;

/* ============================================================
 * GBI Command IDs
 * ============================================================ */
/* RSP geometry commands */
#define G_SPNOOP 0x00
#define G_MTX 0x01
#define G_MOVEMEM 0x03
#define G_VTX 0x04
#define G_DL 0x06
#define G_LOAD_UCODE 0x07
#define G_BRANCH_Z 0x08
#define G_TRI2 0x09
#define G_MODIFYVTX 0x0B
#define G_TRI1 0x0C
#define G_ENDDL 0xDF
#define G_SETOTHERMODE_L 0xE2
#define G_SETOTHERMODE_H 0xE3
#define G_TEXRECT 0xE4
#define G_TEXRECTFLIP 0xE5
#define G_RDPLOADING 0xC0
#define G_SETCIMG 0xFF
#define G_SETZIMG 0xFE
#define G_SETTIMG 0xFD
#define G_SETCOMBINE 0xFC
#define G_SETENVCOLOR 0xFB
#define G_SETPRIMCOLOR 0xFA
#define G_SETBLENDCOLOR 0xF9
#define G_SETFOGCOLOR 0xF8
#define G_SETFILLCOLOR 0xF7
#define G_FILLRECT 0xF6
#define G_SETTILE 0xF5
#define G_LOADTILE 0xF4
#define G_LOADBLOCK 0xF3
#define G_SETTILESIZE 0xF2
#define G_LOADTLUT 0xF0
#define G_RDPSETOTHERMODE 0xEF
#define G_SETPRIMDEPTH 0xEE
#define G_SETSCISSOR 0xED
#define G_SETCONVERT 0xEC
#define G_SETKEYR 0xEB
#define G_SETKEYGB 0xEA
#define G_RDPFULLSYNC 0xE9
#define G_RDPTILESYNC 0xE8
#define G_RDPPIPESYNC 0xE7
#define G_RDPLOADSYNC 0xE6
#define G_NOOP 0xC0
#define G_POPMTX 0xD8
#define G_GEOMETRYMODE 0xD9
#define G_TEXTURE 0xD7
#define G_SETGEOMETRYMODE 0xD9
#define G_CLEARGEOMETRYMODE 0xD9
#define G_LINE3D 0xD1
#define G_MW_NUMLIGHT 0x0C
#define G_MVO_L_BASE 0x8A
#define G_MW_SEGMENT 0x06
#define G_MW_FOG 0x08
#define G_MW_PERSPNORM 0x0E
#define G_MV_VIEWPORT 0x80
#define G_MW_CLIP 0x04

/* Geometry mode flags */
#define G_ZBUFFER 0x00000001
#define G_SHADE 0x00000004
#define G_CULL_FRONT 0x00000200
#define G_CULL_BACK 0x00000400
#define G_CULL_BOTH 0x00000600
#define G_FOG 0x00010000
#define G_LIGHTING 0x00020000
#define G_TEXTURE_GEN 0x00040000
#define G_TEXTURE_GEN_LINEAR 0x00080000
#define G_SHADING_SMOOTH 0x00200000
#define G_CLIPPING 0x00800000

/* Texture filter modes */
#define G_MDSFT_TEXTFILT 12
#define G_TF_POINT 0x0000
#define G_TF_AVERAGE 0x3000
#define G_TF_BILERP 0x2000

/* Image formats */
#define G_IM_FMT_RGBA 0
#define G_IM_FMT_YUV 1
#define G_IM_FMT_CI 2
#define G_IM_FMT_IA 3
#define G_IM_FMT_I 4

/* Image sizes */
#define G_IM_SIZ_4b 0
#define G_IM_SIZ_8b 1
#define G_IM_SIZ_16b 2
#define G_IM_SIZ_32b 3

/* Z mode */
#define Z_UPD 0x00000020
#define ZMODE_DEC 0x00000C00

/* Other mode bits */
#define G_MDSFT_ALPHACOMPARE 0
#define G_MDSFT_ZSRCSEL 2
#define G_MDSFT_RENDERMODE 3

/* Color combiner input mappings */
#define G_CCMUX_COMBINED 0
#define G_CCMUX_TEXEL0 1
#define G_CCMUX_TEXEL1 2
#define G_CCMUX_PRIMITIVE 3
#define G_CCMUX_SHADE 4
#define G_CCMUX_ENVIRONMENT 5
#define G_CCMUX_1 6
#define G_CCMUX_COMBINED_ALPHA 7
#define G_CCMUX_TEXEL0_ALPHA 8
#define G_CCMUX_TEXEL1_ALPHA 9
#define G_CCMUX_PRIMITIVE_ALPHA 10
#define G_CCMUX_SHADE_ALPHA 11
#define G_CCMUX_ENV_ALPHA 12
#define G_CCMUX_LOD_FRACTION 13
#define G_CCMUX_0 31

/* ============================================================
 * GBI Macros — these build display list commands
 * For the Nspire port, we keep NOP versions for now.
 * The actual rendering is dispatched by processing the finished
 * display list through the software renderer.
 * ============================================================ */

/* Append a GBI command to the display list */
#define gDPNoOp(pkt)            \
    {                           \
        Gfx* _g = (Gfx*)(pkt);  \
        _g->w0 = 0;             \
        _g->w1 = 0;             \
        (pkt) = (Gfx*)(_g + 1); \
    }

/* Pipeline/Tile/Load sync — NOPs for software renderer */
#define gDPPipeSync(pkt) gDPNoOp(pkt)
#define gDPTileSync(pkt) gDPNoOp(pkt)
#define gDPLoadSync(pkt) gDPNoOp(pkt)
#define gDPFullSync(pkt) gDPNoOp(pkt)

/* gSPDisplayList — call a child display list */
#define gSPDisplayList(pkt, dl)             \
    {                                       \
        Gfx* _g = (Gfx*)(pkt);              \
        _g->w0 = (G_DL << 24);              \
        _g->w1 = (uint32_t)(uintptr_t)(dl); \
        (pkt) = (Gfx*)(_g + 1);             \
    }

/* gSPEndDisplayList — mark end of display list */
#define gSPEndDisplayList(pkt)    \
    {                             \
        Gfx* _g = (Gfx*)(pkt);    \
        _g->w0 = (G_ENDDL << 24); \
        _g->w1 = 0;               \
        (pkt) = (Gfx*)(_g + 1);   \
    }

/* Vertex loading */
#define gSPVertex(pkt, vtx, n, v0)                              \
    {                                                           \
        Gfx* _g = (Gfx*)(pkt);                                  \
        _g->w0 = (G_VTX << 24) | ((n) << 12) | ((v0 + n) << 1); \
        _g->w1 = (uint32_t)(uintptr_t)(vtx);                    \
        (pkt) = (Gfx*)(_g + 1);                                 \
    }

/* Triangle drawing */
#define gSP1Triangle(pkt, v0, v1, v2, flag)                                            \
    {                                                                                  \
        Gfx* _g = (Gfx*)(pkt);                                                         \
        _g->w0 = (G_TRI1 << 24) | (((v0) * 2) << 16) | (((v1) * 2) << 8) | ((v2) * 2); \
        _g->w1 = 0;                                                                    \
        (pkt) = (Gfx*)(_g + 1);                                                        \
    }

#define gSP2Triangles(pkt, v00, v01, v02, flag0, v10, v11, v12, flag1)                    \
    {                                                                                     \
        Gfx* _g = (Gfx*)(pkt);                                                            \
        _g->w0 = (G_TRI2 << 24) | (((v00) * 2) << 16) | (((v01) * 2) << 8) | ((v02) * 2); \
        _g->w1 = (((v10) * 2) << 16) | (((v11) * 2) << 8) | ((v12) * 2);                  \
        (pkt) = (Gfx*)(_g + 1);                                                           \
    }

/* Matrix operations */
#define gSPMatrix(pkt, mtx, param)                 \
    {                                              \
        Gfx* _g = (Gfx*)(pkt);                     \
        _g->w0 = (G_MTX << 24) | ((param) ^ 0x01); \
        _g->w1 = (uint32_t)(uintptr_t)(mtx);       \
        (pkt) = (Gfx*)(_g + 1);                    \
    }

#define gSPPopMatrix(pkt, param)   \
    {                              \
        Gfx* _g = (Gfx*)(pkt);     \
        _g->w0 = (G_POPMTX << 24); \
        _g->w1 = (param);          \
        (pkt) = (Gfx*)(_g + 1);    \
    }

/* Geometry mode */
#define gSPSetGeometryMode(pkt, mode)                        \
    {                                                        \
        Gfx* _g = (Gfx*)(pkt);                               \
        _g->w0 = (G_GEOMETRYMODE << 24) | (~0 & 0x00FFFFFF); \
        _g->w1 = (uint32_t)(mode);                           \
        (pkt) = (Gfx*)(_g + 1);                              \
    }

#define gSPClearGeometryMode(pkt, mode)                                     \
    {                                                                       \
        Gfx* _g = (Gfx*)(pkt);                                              \
        _g->w0 = (G_GEOMETRYMODE << 24) | (~(uint32_t)(mode) & 0x00FFFFFF); \
        _g->w1 = 0;                                                         \
        (pkt) = (Gfx*)(_g + 1);                                             \
    }

#define gSPLoadGeometryMode(pkt, mode)   \
    {                                    \
        Gfx* _g = (Gfx*)(pkt);           \
        _g->w0 = (G_GEOMETRYMODE << 24); \
        _g->w1 = (uint32_t)(mode);       \
        (pkt) = (Gfx*)(_g + 1);          \
    }

/* Texture enable */
#define gSPTexture(pkt, sc, tc, level, tile, on)                             \
    {                                                                        \
        Gfx* _g = (Gfx*)(pkt);                                               \
        _g->w0 = (G_TEXTURE << 24) | ((level) << 11) | ((tile) << 8) | (on); \
        _g->w1 = ((sc) << 16) | (tc);                                        \
        (pkt) = (Gfx*)(_g + 1);                                              \
    }

/* Set color combiner */
#define gDPSetCombineLERP(pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    {                                                                                                  \
        Gfx* _g = (Gfx*)(pkt);                                                                         \
        _g->w0 = (G_SETCOMBINE << 24);                                                                 \
        _g->w1 = 0;                                                                                    \
        (pkt) = (Gfx*)(_g + 1);                                                                        \
    }

/* Colors */
#define gDPSetEnvColor(pkt, r, g, b, a)                        \
    {                                                          \
        Gfx* _g = (Gfx*)(pkt);                                 \
        _g->w0 = (G_SETENVCOLOR << 24);                        \
        _g->w1 = ((r) << 24) | ((g) << 16) | ((b) << 8) | (a); \
        (pkt) = (Gfx*)(_g + 1);                                \
    }

#define gDPSetPrimColor(pkt, m, l, r, g, b, a)                 \
    {                                                          \
        Gfx* _g = (Gfx*)(pkt);                                 \
        _g->w0 = (G_SETPRIMCOLOR << 24) | ((m) << 8) | (l);    \
        _g->w1 = ((r) << 24) | ((g) << 16) | ((b) << 8) | (a); \
        (pkt) = (Gfx*)(_g + 1);                                \
    }

#define gDPSetFogColor(pkt, r, g, b, a)                        \
    {                                                          \
        Gfx* _g = (Gfx*)(pkt);                                 \
        _g->w0 = (G_SETFOGCOLOR << 24);                        \
        _g->w1 = ((r) << 24) | ((g) << 16) | ((b) << 8) | (a); \
        (pkt) = (Gfx*)(_g + 1);                                \
    }

#define gDPSetFillColor(pkt, c)          \
    {                                    \
        Gfx* _g = (Gfx*)(pkt);           \
        _g->w0 = (G_SETFILLCOLOR << 24); \
        _g->w1 = (c);                    \
        (pkt) = (Gfx*)(_g + 1);          \
    }

/* Scissors */
#define gDPSetScissor(pkt, mode, ulx, uly, lrx, lry)           \
    {                                                          \
        Gfx* _g = (Gfx*)(pkt);                                 \
        _g->w0 = (G_SETSCISSOR << 24) | ((ulx) << 12) | (uly); \
        _g->w1 = ((mode) << 24) | ((lrx) << 12) | (lry);       \
        (pkt) = (Gfx*)(_g + 1);                                \
    }

/* Fill rect */
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry)            \
    {                                                        \
        Gfx* _g = (Gfx*)(pkt);                               \
        _g->w0 = (G_FILLRECT << 24) | ((lrx) << 12) | (lry); \
        _g->w1 = ((ulx) << 12) | (uly);                      \
        (pkt) = (Gfx*)(_g + 1);                              \
    }

/* Texture image/tile setup */
#define gDPSetTextureImage(pkt, fmt, siz, width, i)                                 \
    {                                                                               \
        Gfx* _g = (Gfx*)(pkt);                                                      \
        _g->w0 = (G_SETTIMG << 24) | ((fmt) << 21) | ((siz) << 19) | ((width) - 1); \
        _g->w1 = (uint32_t)(uintptr_t)(i);                                          \
        (pkt) = (Gfx*)(_g + 1);                                                     \
    }

#define gDPSetTile(pkt, fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts)       \
    {                                                                                                      \
        Gfx* _g = (Gfx*)(pkt);                                                                             \
        _g->w0 = (G_SETTILE << 24) | ((fmt) << 21) | ((siz) << 19) | ((line) << 9) | (tmem);               \
        _g->w1 = ((tile) << 24) | ((palette) << 20) | ((cmt) << 18) | ((maskt) << 14) | ((shiftt) << 10) | \
                 ((cms) << 8) | ((masks) << 4) | (shifts);                                                 \
        (pkt) = (Gfx*)(_g + 1);                                                                            \
    }

#define gDPLoadBlock(pkt, tile, uls, ult, lrs, dxt)           \
    {                                                         \
        Gfx* _g = (Gfx*)(pkt);                                \
        _g->w0 = (G_LOADBLOCK << 24) | ((uls) << 12) | (ult); \
        _g->w1 = ((tile) << 24) | ((lrs) << 12) | (dxt);      \
        (pkt) = (Gfx*)(_g + 1);                               \
    }

#define gDPLoadTile(pkt, tile, uls, ult, lrs, lrt)           \
    {                                                        \
        Gfx* _g = (Gfx*)(pkt);                               \
        _g->w0 = (G_LOADTILE << 24) | ((uls) << 12) | (ult); \
        _g->w1 = ((tile) << 24) | ((lrs) << 12) | (lrt);     \
        (pkt) = (Gfx*)(_g + 1);                              \
    }

#define gDPSetTileSize(pkt, tile, uls, ult, lrs, lrt)           \
    {                                                           \
        Gfx* _g = (Gfx*)(pkt);                                  \
        _g->w0 = (G_SETTILESIZE << 24) | ((uls) << 12) | (ult); \
        _g->w1 = ((tile) << 24) | ((lrs) << 12) | (lrt);        \
        (pkt) = (Gfx*)(_g + 1);                                 \
    }

#define gDPLoadTLUT_pal256(pkt, dram)         \
    {                                         \
        Gfx* _g = (Gfx*)(pkt);                \
        _g->w0 = (G_LOADTLUT << 24);          \
        _g->w1 = (uint32_t)(uintptr_t)(dram); \
        (pkt) = (Gfx*)(_g + 1);               \
    }

/* Other mode */
#define gDPSetOtherMode(pkt, modeH, modeL)            \
    {                                                 \
        Gfx* _g = (Gfx*)(pkt);                        \
        _g->w0 = (G_RDPSETOTHERMODE << 24) | (modeH); \
        _g->w1 = (modeL);                             \
        (pkt) = (Gfx*)(_g + 1);                       \
    }

/* Segment addressing */
#define gSPSegment(pkt, seg, base)            \
    {                                         \
        Gfx* _g = (Gfx*)(pkt);                \
        _g->w0 = (G_MOVEMEM << 24);           \
        _g->w1 = (uint32_t)(uintptr_t)(base); \
        (pkt) = (Gfx*)(_g + 1);               \
    }

/* Viewport */
#define gSPViewport(pkt, v)                \
    {                                      \
        Gfx* _g = (Gfx*)(pkt);             \
        _g->w0 = (G_MOVEMEM << 24);        \
        _g->w1 = (uint32_t)(uintptr_t)(v); \
        (pkt) = (Gfx*)(_g + 1);            \
    }

/* Lights */
#define gSPNumLights(pkt, n) gDPNoOp(pkt)
#define gSPLight(pkt, l, n) gDPNoOp(pkt)
#define gSPSetLights1(pkt, l) gDPNoOp(pkt)
#define gSPSetLights7(pkt, l) gDPNoOp(pkt)

/* Fog */
#define gSPFogPosition(pkt, min, max) gDPNoOp(pkt)

/* Texture rectangle */
#define gSPTextureRectangle(pkt, ulx, uly, lrx, lry, tile, s, t, dsdx, dtdy) \
    {                                                                        \
        Gfx* _g = (Gfx*)(pkt);                                               \
        _g->w0 = (G_TEXRECT << 24) | ((lrx) << 12) | (lry);                  \
        _g->w1 = ((tile) << 24) | ((ulx) << 12) | (uly);                     \
        (pkt) = (Gfx*)(_g + 1);                                              \
        _g = (Gfx*)(pkt);                                                    \
        _g->w0 = ((s) << 16) | (t);                                          \
        _g->w1 = ((dsdx) << 16) | (dtdy);                                    \
        (pkt) = (Gfx*)(_g + 1);                                              \
    }

/* Color/Z image targets */
#define gDPSetColorImage(pkt, fmt, siz, width, i)                                   \
    {                                                                               \
        Gfx* _g = (Gfx*)(pkt);                                                      \
        _g->w0 = (G_SETCIMG << 24) | ((fmt) << 21) | ((siz) << 19) | ((width) - 1); \
        _g->w1 = (uint32_t)(uintptr_t)(i);                                          \
        (pkt) = (Gfx*)(_g + 1);                                                     \
    }

#define gDPSetDepthImage(pkt, i)           \
    {                                      \
        Gfx* _g = (Gfx*)(pkt);             \
        _g->w0 = (G_SETZIMG << 24);        \
        _g->w1 = (uint32_t)(uintptr_t)(i); \
        (pkt) = (Gfx*)(_g + 1);            \
    }

/* Perspective normalization */
#define gSPPerspNormalize(pkt, s) gDPNoOp(pkt)

/* Misc convenience aliases used by MM */
#define gSPBranchList(pkt, dl) gSPDisplayList(pkt, dl)
#define G_MTX_MODELVIEW 0x00
#define G_MTX_PROJECTION 0x04
#define G_MTX_MUL 0x00
#define G_MTX_LOAD 0x02
#define G_MTX_NOPUSH 0x00
#define G_MTX_PUSH 0x01

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

/* N64 render mode macros — stubbed */
#define G_RM_AA_ZB_XLU_SURF(clk) 0
#define G_RM_AA_ZB_OPA_SURF(clk) 0
#define G_RM_AA_ZB_DEC_LINE(clk) 0
#define G_RM_FOG_SHADE_A 0
#define G_RM_PASS 0

/* ARRAY_COUNT macro */
#ifndef ARRAY_COUNT
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/* UNUSED macro */
#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

#endif /* GBI_NSP_H */
