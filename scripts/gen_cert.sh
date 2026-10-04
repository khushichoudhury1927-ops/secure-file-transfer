#!/usr/bin/env bash
set -e
mkdir -p tls_key
cat > /tmp/san.cnf <<'CNF'
[req]
distinguished_name=dn
x509_extensions=v3
prompt=no
[dn]
CN=localhost
[v3]
subjectAltName=DNS:localhost,IP:127.0.0.1
CNF
openssl req -x509 -newkey rsa:2048 -nodes -keyout tls_key/server.key -out tls_key/server.crt -days 365 -config /tmp/san.cnf
rm /tmp/san.cnf
echo "Created tls_key/server.crt and tls_key/server.key"
