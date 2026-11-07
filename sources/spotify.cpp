#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <tremor/ivorbisfile.h>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
#define SAMPLERATE      44100
#define BUF_SIZE        (4096 * 4)

static u32 *socBuffer = NULL;
static int sock_fd = -1;

size_t net_read(void *ptr, size_t size, size_t nmemb, void *datasource) {
    int fd = *(int*)datasource;
    ssize_t res = recv(fd, ptr, size * nmemb, 0);
    if (res <= 0) return 0;
    return res / size;
}

int net_close(void *datasource) {
    int fd = *(int*)datasource;
    if (fd >= 0) close(fd);
    return 0;
}

int connect_http(const char* ip, int port, const char* track_id) {
    struct sockaddr_in serv_addr;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) return -1;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock_fd);
        sock_fd = -1;
        return -1;
    }

    char request[512];
    snprintf(request, sizeof(request),
        "GET /music/spotify:track:%s/ HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "User-Agent: 3DS-Spotify/1.0\r\n"
        "Connection: close\r\n\r\n",
        track_id, ip, port);

    send(sock_fd, request, strlen(request), 0);

    char buffer[1];
    int header_state = 0;
    while (recv(sock_fd, buffer, 1, 0) > 0) {
        if ((header_state == 0 || header_state == 2) && buffer[0] == '\r') header_state++;
        else if ((header_state == 1 || header_state == 3) && buffer[0] == '\n') header_state++;
        else header_state = 0;

        if (header_state == 4) break;
    }

    return sock_fd;
}

void get_user_input(const char* hint, char* out_buf, size_t max_len) {
    SwkbdState swkbd;
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
    swkbdSetHintText(&swkbd, hint);
    swkbdInputText(&swkbd, out_buf, max_len);
}

int main(int argc, char **argv) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

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

    char server_ip[32] = "192.168.086.109";
    char track_id[64] = "2FpEXjGqe2dJJ9oB8c8Io2";
    bool playing = false;

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
    printf("        3ds Spotify!!              \n");
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

        if (kDown & KEY_START) break;

        if (kDown & KEY_A) {
            get_user_input("Enter Server IP Address", server_ip, sizeof(server_ip));
            printf("Server IP set to: %s\n", server_ip);
        }

        if (kDown & KEY_Y) {
            get_user_input("Enter Track ID", track_id, sizeof(track_id));
            printf("Track ID set to: %s\n", track_id);
        }

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
        }

        if (kDown & KEY_B) {
            if (playing) {
                ndspChnWaveBufClear(0);
                ov_clear(&vf);
                playing = false;
                printf("Stopped playback.\n");
            }
        }

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
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    if (playing) ov_clear(&vf);

    linearFree(pcm_buffers[0]);
    linearFree(pcm_buffers[1]);

    ndspExit();
    socExit();
    if (socBuffer) free(socBuffer);
    gfxExit();

    return 0;
}