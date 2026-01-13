# Installation
Linux
```bash
sudo apt-get update
sudo apt-get install libssl-dev
```

MacOS
```bash
brew install openssl@3
# Nota: macOS ha una vecchia versione di LibreSSL di default. 
# Potrebbe essere necessario dire al compilatore dove trovare OpenSSL di brew nel Makefile:
# CFLAGS += -I/opt/homebrew/opt/openssl@3/include
# LDFLAGS += -L/opt/homebrew/opt/openssl@3/lib
```