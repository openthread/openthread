#!/bin/sh

REPO_DIR=repo.patched

rm -r ${REPO_DIR}
mkdir ${REPO_DIR}

curl -L http://github.com/ARMmbed/mbedtls/archive/mbedtls-2.4.1.tar.gz | tar zx - -C ${REPO_DIR} --strip-components=1

patch ${REPO_DIR}/library/ssl_tls.c patch/0001-Fix-commissioning-problem-with-retransmissions.patch
patch ${REPO_DIR}/library/ssl_srv.c patch/0002-Fix-warning-dead-store.patch 
