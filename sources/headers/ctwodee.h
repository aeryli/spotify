#include <3ds.h>
#include <cstddef>
#include <stdio.h>
#include <citro2d.h>
//'screens' is just {C3D_RenderTarget* top; C3D_RenderTarget* bottom;}
struct screens {C3D_RenderTarget* top; C3D_RenderTarget* bottom;};
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
};
void fin_frame() {
    C3D_FrameEnd(0);
};

void fin_loop() {
    C2D_Fini();
    C3D_Fini();
};
