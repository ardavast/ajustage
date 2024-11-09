#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <spandsp.h>
#include <portaudio.h>

void audio_init()
{
    int ostderr = dup(STDERR_FILENO);
    int devnull = open("/dev/null", O_WRONLY, 0600);
    dup2(devnull, STDERR_FILENO);

    // Sandwhich Pa_Initialize between these redirections of stderr in order
    // to avoid annoying ALSA messages on the screen.
    PaError err = Pa_Initialize();

    dup2(ostderr, STDERR_FILENO);
    close(devnull);

    if (err != paNoError) {
        fprintf(stderr, "Pa_Initialize() %d: %s\n", err, Pa_GetErrorText(err));
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    PaError err;
    const PaDeviceInfo *deviceInfo;

    audio_init();

    int ndev = Pa_GetDeviceCount();
    if (ndev < 0) {
        err = ndev;
        Pa_Terminate();
        fprintf(stderr, "Pa_GetDeviceCount() %d: %s\n", err, Pa_GetErrorText(err));
        return 1;
    }

    for (int i = 0; i < ndev; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo->maxInputChannels > 0) {
            printf("Input Device #%d: %s\n", i, deviceInfo->name);
        }
    }

    for (int i = 0; i < ndev; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo->maxOutputChannels > 0) {
            printf("Output Device #%d: %s\n", i, deviceInfo->name);
        }
    }

    Pa_Terminate();
    return 0;
}
