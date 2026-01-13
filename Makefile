CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./common -D_POSIX_C_SOURCE=200809L
LDFLAGS_COMMON = -lssl -lcrypto

# Rilevamento OS
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    TARGET = open-kvm-client
    SRC = client/main.c client/capturer.c common/net_utils.c
    LDFLAGS = $(LDFLAGS_COMMON)
endif

ifeq ($(UNAME_S),Darwin)
    TARGET = open-kvm-server
    SRC = server/main.c server/injector.c server/keymap.c common/net_utils.c
    
    # macOS: Cerca OpenSSL in Homebrew (Silicon o Intel)
    ifneq ($(wildcard /opt/homebrew/opt/openssl@3/include),)
        CFLAGS += -I/opt/homebrew/opt/openssl@3/include
        LDFLAGS += -L/opt/homebrew/opt/openssl@3/lib
    else ifneq ($(wildcard /usr/local/opt/openssl@3/include),)
        CFLAGS += -I/usr/local/opt/openssl@3/include
        LDFLAGS += -L/usr/local/opt/openssl@3/lib
    endif
    
    LDFLAGS += $(LDFLAGS_COMMON) -framework ApplicationServices
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	sleep 1 #rm -f open-kvm-client open-kvm-server
