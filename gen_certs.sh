#!/bin/bash
mkdir -p certs/keys/
# Genera chiave privata e certificato autofirmato per il server (validità 365 giorni)
openssl req -x509 -newkey rsa:4096 -keyout certs/keys/server.key -out certs/keys/server.crt -days 365 -nodes -subj "/CN=localhost"
echo "Certificates generated in certs/keys/"
