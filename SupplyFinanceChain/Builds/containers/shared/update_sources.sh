#!/usr/bin/env bash

function error {
    echo $1
    exit 1
}

cd /opt/SupplyFinanceChaind_bld/pkg/SupplyFinanceChaind
export SUPPLYFINANCECHAIND_VERSION=$(egrep -i -o "\b(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(-[0-9a-z\-]+(\.[0-9a-z\-]+)*)?(\+[0-9a-z\-]+(\.[0-9a-z\-]+)*)?\b" src/SupplyFinanceChain/protocol/impl/BuildInfo.cpp)

cd ..
git clone https://github.com/SupplyFinanceChain/validator-keys-tool.git
cd validator-keys-tool
git checkout origin/master
git submodule update --init --recursive
cd ..

: ${PKG_OUTDIR:=/opt/SupplyFinanceChaind_bld/pkg/out}
export PKG_OUTDIR
if [ ! -d ${PKG_OUTDIR} ]; then
    error "${PKG_OUTDIR} is not mounted"
fi

if [ -x ${OPENSSL_ROOT}/bin/openssl ]; then
    LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${OPENSSL_ROOT}/lib ${OPENSSL_ROOT}/bin/openssl version -a
fi

