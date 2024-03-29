#!/usr/bin/env bash
set -ex

source update_sources.sh

# Build the dpkg

#dpkg uses - as separator, so we need to change our -bN versions to tilde
SUPPLYFINANCECHAIND_DPKG_VERSION=$(echo "${SUPPLYFINANCECHAIND_VERSION}" | sed 's!-!~!g')
# TODO - decide how to handle the trailing/release
# version here (hardcoded to 1). Does it ever need to change?
SUPPLYFINANCECHAIND_DPKG_FULL_VERSION="${SUPPLYFINANCECHAIND_DPKG_VERSION}-1"

cd SupplyFinanceChaind
if [[ -n $(git status --porcelain) ]]; then
    git status
    error "Unstaged changes in this repo - please commit first"
fi
git archive --format tar.gz --prefix SupplyFinanceChaind-${SUPPLYFINANCECHAIND_DPKG_VERSION}/ -o ../SupplyFinanceChaind-${SUPPLYFINANCECHAIND_DPKG_VERSION}.tar.gz HEAD
cd ..
# dpkg debmake would normally create this link, but we do it manually
ln -s ./SupplyFinanceChaind-${SUPPLYFINANCECHAIND_DPKG_VERSION}.tar.gz SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_VERSION}.orig.tar.gz
tar xvf SupplyFinanceChaind-${SUPPLYFINANCECHAIND_DPKG_VERSION}.tar.gz
cd SupplyFinanceChaind-${SUPPLYFINANCECHAIND_DPKG_VERSION}
cp -pr ../debian .

# dpkg requires a changelog. We don't currently maintain
# a useable one, so let's just fake it with our current version
# TODO : not sure if the "unstable" will need to change for
# release packages (?)
NOWSTR=$(TZ=UTC date -R)
cat << CHANGELOG > ./debian/changelog
SupplyFinanceChaind (${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}) unstable; urgency=low

  * see RELEASENOTES

 -- SupplyFinanceChain Labs Inc. <support@SupplyFinanceChain.com>  ${NOWSTR}
CHANGELOG

# PATH must be preserved for our more modern cmake in /opt/local
# TODO : consider allowing lintian to run in future ?
export DH_BUILD_DDEBS=1
debuild --no-lintian --preserve-envvar PATH --preserve-env -us -uc
rc=$?; if [[ $rc != 0 ]]; then
    error "error building dpkg"
fi
cd ..
ls -latr

# copy artifacts
cp SupplyFinanceChaind-dev_${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}_amd64.deb ${PKG_OUTDIR}
cp SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}_amd64.deb ${PKG_OUTDIR}
cp SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}.dsc ${PKG_OUTDIR}
# dbgsym suffix is ddeb under newer debuild, but just deb under earlier
cp SupplyFinanceChaind-dbgsym_${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}_amd64.* ${PKG_OUTDIR}
cp SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}_amd64.changes ${PKG_OUTDIR}
cp SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}_amd64.build ${PKG_OUTDIR}
cp SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_VERSION}.orig.tar.gz ${PKG_OUTDIR}
cp SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}.debian.tar.xz ${PKG_OUTDIR}
# buildinfo is only generated by later version of debuild
if [ -e SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}_amd64.buildinfo ] ; then
    cp SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}_amd64.buildinfo ${PKG_OUTDIR}
fi

cat SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}_amd64.changes
# extract the text in the .changes file that appears between
#    Checksums-Sha256:  ...
# and
#    Files: ...
awk '/Checksums-Sha256:/{hit=1;next}/Files:/{hit=0}hit' \
    SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_VERSION}-1_amd64.changes | \
        sed -E 's!^[[:space:]]+!!' > shasums
DEB_SHA256=$(cat shasums | \
    grep "SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_VERSION}-1_amd64.deb" | cut -d " " -f 1)
DBG_SHA256=$(cat shasums | \
    grep "SupplyFinanceChaind-dbgsym_${SUPPLYFINANCECHAIND_DPKG_VERSION}-1_amd64.*" | cut -d " " -f 1)
DEV_SHA256=$(cat shasums | \
    grep "SupplyFinanceChaind-dev_${SUPPLYFINANCECHAIND_DPKG_VERSION}-1_amd64.deb" | cut -d " " -f 1)
SRC_SHA256=$(cat shasums | \
    grep "SupplyFinanceChaind_${SUPPLYFINANCECHAIND_DPKG_VERSION}.orig.tar.gz" | cut -d " " -f 1)
echo "deb_sha256=${DEB_SHA256}" >> ${PKG_OUTDIR}/build_vars
echo "dbg_sha256=${DBG_SHA256}" >> ${PKG_OUTDIR}/build_vars
echo "dev_sha256=${DEV_SHA256}" >> ${PKG_OUTDIR}/build_vars
echo "src_sha256=${SRC_SHA256}" >> ${PKG_OUTDIR}/build_vars
echo "SupplyFinanceChaind_version=${SUPPLYFINANCECHAIND_VERSION}" >> ${PKG_OUTDIR}/build_vars
echo "dpkg_version=${SUPPLYFINANCECHAIND_DPKG_VERSION}" >> ${PKG_OUTDIR}/build_vars
echo "dpkg_full_version=${SUPPLYFINANCECHAIND_DPKG_FULL_VERSION}" >> ${PKG_OUTDIR}/build_vars

