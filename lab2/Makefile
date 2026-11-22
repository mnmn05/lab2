# lab2 전체 빌드:  make        정리: make clean
CC      = gcc
CFLAGS  = -Wall -Wextra -g
DBUS    = $(shell pkg-config --cflags --libs dbus-1 2>/dev/null)

BINS = 02_file/file_ops 03_lsr/myls_r 04_typing/typing \
       05_process_signal/proc_demo 05_process_signal/signal_demo \
       06_ipc/pipe_demo 06_ipc/fifo_demo 06_ipc/sysv_ipc_demo \
       07_pipe_chat/pipe_chat 08_mq_chat/mq_chat 09_shm_copy/shm_copy \
       10_dbus/notify_server 10_dbus/notify_client

all: $(BINS) shell rust

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

08_mq_chat/mq_chat: 08_mq_chat/mq_chat.c
	$(CC) $(CFLAGS) -o $@ $< -lrt

09_shm_copy/shm_copy: 09_shm_copy/shm_copy.c
	$(CC) $(CFLAGS) -o $@ $< -pthread -lrt

10_dbus/notify_server: 10_dbus/notify_server.c 10_dbus/notify_common.h
	$(CC) $(CFLAGS) -o $@ $< $(DBUS)

10_dbus/notify_client: 10_dbus/notify_client.c 10_dbus/notify_common.h
	$(CC) $(CFLAGS) -o $@ $< $(DBUS)

shell:
	$(MAKE) -C 11_myshell

rust:
	cd 01_matrix_rust && cargo build --release

clean:
	rm -f $(BINS)
	$(MAKE) -C 11_myshell clean
	cd 01_matrix_rust && cargo clean

.PHONY: all shell rust clean
