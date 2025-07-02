#!/bin/bash

git clone https://github.com/mackyle/xar.git
cd xar
git checkout 66d451dab1ef859dd0c83995f2379335d35e53c9

cd xar
sed -i 's/OpenSSL_add_all_ciphers/OPENSSL_init_crypto/g' configure.ac
./autogen.sh --noconfigure
./configure
make
