#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <spandsp.h>
#include <portaudio.h>

dtmf_tx_state_t *gen;

void pa_err(PaError err, const char *fn)
{
    fprintf(stderr, "%s %d: %s\n", fn, err, Pa_GetErrorText(err));
}

void audio_init()
{
    int ostderr = dup(STDERR_FILENO);
    int devnull = open("/dev/null", O_WRONLY, 0600);
    dup2(devnull, STDERR_FILENO);

    // Sandwich Pa_Initialize() between these redirections of stderr in order
    // to avoid annoying ALSA messages on the screen
    PaError err = Pa_Initialize();

    dup2(ostderr, STDERR_FILENO);
    close(devnull);

    if (err != paNoError) {
        pa_err(err, "Pa_Initialize()");
        exit(1);
    }
}

static int audio_callback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData)
{
    int16_t *out = (int16_t *)outputBuffer;

    int len = dtmf_tx(gen, out, framesPerBuffer);

    // zero the remaining buffer
    for (unsigned int i = len; i < framesPerBuffer; i++) {
        out[i] = 0;
    }

    return paContinue;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: dtmf_tx <output device>\n");
	return 1;
    }

    gen = dtmf_tx_init(NULL, NULL, NULL);

    audio_init();

    PaStreamParameters out_params;
    PaStream *stream;
    PaError err;

    errno = 0;
    out_params.device = strtol(argv[1], NULL, 10);
    if (errno) {
        fprintf(stderr, "usage: dtmf_tx <output device>\n");
	return 1;
    }

    out_params.channelCount = 1;
    out_params.sampleFormat = paInt16;

    err = Pa_OpenStream(
              &stream,        // stresam
              NULL,           // input
              &out_params,    // output
              8000,           // sample rate
              256,            // buffer size, 256 = ~5ms
              paClipOff,      // flags
              audio_callback, // callback
              NULL);          // callback data

    if (err != paNoError) {
      pa_err(err, "Pa_OpenStream()");
      Pa_Terminate();
      exit(1);
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
      pa_err(err, "Pa_StartStream()");
      Pa_Terminate();
      exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    printf("> ");
    fflush(stdout);

    while ((nread = getline(&line, &len, stdin)) != -1) {
        if (strlen(line) > 128) {
            line[128] = '\0';
	}

        dtmf_tx_put(gen, line, -1);
        printf("> ");
        fflush(stdout);
    }

    return 0;
}
