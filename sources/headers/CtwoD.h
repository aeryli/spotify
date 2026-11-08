#include <3ds.h>
#include <cstddef>
#include <stdio.h>
#include <citro2d.h>
//holds screens' render targets
struct screens {C3D_RenderTarget* top; C3D_RenderTarget* bottom;};
//initialize screens, save space.
screens setup(bool topp, bool bottomm) {
    gfxInitDefault();
    //gfxSet3D(false);
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    screens outp;
    if (topp) {
        outp.top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    } else {
        consoleInit(GFX_TOP, NULL);
    };
    if (bottomm) {
        outp.bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    } else {
        consoleInit(GFX_BOTTOM, NULL);
    };
    return outp;
}
//begin `C3D` (?) frame
void begin_frame(C3D_RenderTarget *screenIn, u32 color) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(screenIn, color);
    C2D_SceneBegin(screenIn);
}
//finish `C3D` (?) frame
void fin_frame() {
    C3D_FrameEnd(0);
}
//exit code for `C2D`/`C3D`
void fin_loop() {
    C2D_Fini();
    C3D_Fini();
}
