all: dtmf_tx list_devs

dtmf_tx: dtmf_tx.c
	$(CC) -o $@ $< -lspandsp -lportaudio

list_devs: list_devs.c
	$(CC) -o $@ $< -lspandsp -lportaudio

clean:
	rm -f dtmf_tx list_devs
