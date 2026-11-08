#include <3ds.h>
#include <string>
#include <netdb.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <citro2d.h>
#include "headers/json.h"
#include "headers/https.h"
#include "headers/CtwoD.h"
#include "tremor/ivorbisfile.h"

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
#define SAMPLERATE      44100
#define BUF_SIZE        (1024 * 16)

//comment!!

void get_user_input(const char* hint, char* out_buf, size_t max_len) {
    SwkbdState swkbd;
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
    swkbdSetHintText(&swkbd, hint);
    swkbdInputText(&swkbd, out_buf, max_len);
}

int main(int argc, char **argv) {
    screens screens = setup(true, false);
    C2D_TargetClear(screens.top, C2D_Color32(0, 255, 0, 255));

    socBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (socBuffer == NULL || R_FAILED(socInit(socBuffer, SOC_BUFFERSIZE))) {
        printf("Failed to initialize SOC service!\n");
        while (aptMainLoop()) {
            gspWaitForVBlank();
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
        }
        return 0;
    }

    // Initialize NDSP service
    if (R_FAILED(ndspInit())) {
        printf("Failed to initialize NDSP service!\n");
    }

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, SAMPLERATE);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

    // Prepare double-buffer structures for NDSP
    ndspWaveBuf waveBuf[2];
    u8 *pcm_buffers[2];

    memset(waveBuf, 0, sizeof(waveBuf));
    pcm_buffers[0] = (u8*)linearAlloc(BUF_SIZE);
    pcm_buffers[1] = (u8*)linearAlloc(BUF_SIZE);

    waveBuf[0].data_vaddr = pcm_buffers[0];
    waveBuf[0].nsamples = BUF_SIZE / 4; // 16-bit stereo = 4 bytes per sample frame
    
    waveBuf[1].data_vaddr = pcm_buffers[1];
    waveBuf[1].nsamples = BUF_SIZE / 4;

    char  server_ip[32] = "192.168.086.109";
    char  track_id[64] = "2FpEXjGqe2dJJ9oB8c8Io2";
    const size_t id_size = (8 * 1024 * sizeof(char));
    char* search_ids = (char*)malloc(id_size);
    json_root json_ids;
    char search_query[64] = "";
    bool playing = false;
    const char *label = "";
    size_t label_len;

    OggVorbis_File vf;
    ov_callbacks callbacks = {
        net_read,
        NULL,
        net_close,
        NULL
    };

    int current_section = 0;
    int buf_idx = 0;

    printf("===================================\n");
    printf("            3ds Spotify!!          \n");
    printf("===================================\n");
    printf("Controls:\n");
    printf("  [A] Set Server IP\n");
    printf("  [Y] Set Track ID\n");
    printf("  [X] Play Stream\n");
    printf("  [B] Stop Stream\n");
    printf("  [START] Exit Application\n");
    printf("-----------------------------------\n\n");

    while (aptMainLoop()) {

        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) {
            free(search_ids); 
            break;
        }

        if (kDown & KEY_A) {
            get_user_input("Enter Server IP Address", server_ip, sizeof(server_ip));
            printf("Server IP set to: %s\n", server_ip);
        }

        if (kDown & KEY_Y) {
            get_user_input("Enter a search query.", search_query, sizeof(track_id));
            printf("Searching: %s\n", search_query);
            printf("%d", search_http(search_ids, id_size, server_ip, 8000, search_query));
            //printf(search_ids);
            json_error err_jsn;
            if (json_parse(search_ids, &json_ids, &err_jsn) != JSON_OK) {
                fprintf(stderr, "parse error at byte %lu\n", (unsigned long)err_jsn.offset);
                return 1;
            } else {
                printf("json good!\n");
            };
            //printf((char*)status);
            for(int i = 0; i<5; i++) {
                std::string strinng = std::to_string(i) + ".name";
                if (json_path_get_string(&json_ids.value, strinng.c_str(), &label, &label_len) != JSON_OK) {
                    fprintf(stderr, "parse error at byte %lu\n", (unsigned long)err_jsn.offset);
                    return 1;
                } else {
                    printf("\n%s\n", label);
                };
            };
            printf("done!!");
        };

        if (kDown & KEY_X) {
            if (playing) {
                ndspChnWaveBufClear(0);
                ov_clear(&vf);
                playing = false;
            }

            printf("\nConnecting to %s:8000...\n", server_ip);
            int fd = connect_http(server_ip, 8000, track_id);
            if (fd >= 0) {
                if (ov_open_callbacks(&sock_fd, &vf, NULL, 0, callbacks) < 0) {
                    printf("Error: Could not decode Ogg Vorbis stream.\n");
                    close(fd);
                } else {
                    printf("Playing: spotify:track:%s\n", track_id);
                    playing = true;
                }
            } else {
                printf("Error: Connection failed!\n");
            }
        };

        if (kDown & KEY_B) {
            if (playing) {
                ndspChnWaveBufClear(0);
                ov_clear(&vf);
                playing = false;
                printf("Stopped playback.\n");
            }
        };

        if (playing) {
            if (waveBuf[buf_idx].status == NDSP_WBUF_DONE || waveBuf[buf_idx].status == NDSP_WBUF_FREE) {
                long ret = ov_read(&vf, (char*)pcm_buffers[buf_idx], BUF_SIZE, &current_section);
                if (ret > 0) {
                    DSP_FlushDataCache(pcm_buffers[buf_idx], ret);
                    waveBuf[buf_idx].nsamples = ret / 4;
                    ndspChnWaveBufAdd(0, &waveBuf[buf_idx]);
                    buf_idx = !buf_idx;
                } else if (ret == 0) {
                    printf("Stream finished.\n");
                    ov_clear(&vf);
                    playing = false;
                }
            }
        };
        fin_frame();
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    };
    fin_loop();
    if (playing) ov_clear(&vf);

    linearFree(pcm_buffers[0]);
    linearFree(pcm_buffers[1]);

    ndspExit();
    socExit();
    if (socBuffer) free(socBuffer);
    gfxExit();

    return 0;
}