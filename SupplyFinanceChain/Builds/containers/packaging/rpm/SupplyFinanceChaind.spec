%define SupplyFinanceChaind_version %(echo $SUPPLYFINANCECHAIND_RPM_VERSION)
%define rpm_release %(echo $RPM_RELEASE)
%define rpm_patch %(echo $RPM_PATCH)
%define _prefix /opt/SupplyFinanceChain
Name:           SupplyFinanceChaind
# Dashes in Version extensions must be converted to underscores
Version:        %{SupplyFinanceChaind_version}
Release:        %{rpm_release}%{?dist}%{rpm_patch}
Summary:        SupplyFinanceChaind daemon

License:        MIT
URL:            http://SupplyFinanceChain.com/
Source0:        SupplyFinanceChaind.tar.gz
Source1:        validator-keys.tar.gz

BuildRequires:  protobuf-static openssl-static cmake zlib-static ninja-build

%description
SupplyFinanceChaind

%package devel
Summary: Files for development of applications using sfcl core library
Group: Development/Libraries
Requires: openssl-static, zlib-static

%description devel
core library for development of standalone applications that sign transactions.

%prep
%setup -c -n SupplyFinanceChaind -a 1

%build
cd SupplyFinanceChaind
mkdir -p bld.release
cd bld.release
cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_BUILD_TYPE=Release -Dstatic=true -DCMAKE_VERBOSE_MAKEFILE=ON -Dlocal_protobuf=ON
# build VK
cd ../../validator-keys-tool
mkdir -p bld.release
cd bld.release
# Install a copy of the SupplyFinanceChaind artifacts into a local VK build dir so that it
# can use them to build against (VK needs sfcl_core lib to build). We install
# into a local build dir instead of buildroot because we want VK to have
# relative paths embedded in debug info, otherwise check-buildroot (rpmbuild)
# will complain
mkdir sfcl_dir
DESTDIR="%{_builddir}/validator-keys-tool/bld.release/sfcl_dir"  cmake --build ../../SupplyFinanceChaind/bld.release --target install -- -v
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%{_builddir}/validator-keys-tool/bld.release/sfcl_dir/opt/SupplyFinanceChain -Dstatic=true -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build . --parallel -- -v

%pre
test -e /etc/pki/tls || { mkdir -p /etc/pki; ln -s /usr/lib/ssl /etc/pki/tls; }

%install
rm -rf $RPM_BUILD_ROOT
DESTDIR=$RPM_BUILD_ROOT cmake --build SupplyFinanceChaind/bld.release --target install -- -v
install -d ${RPM_BUILD_ROOT}/etc/opt/SupplyFinanceChain
install -d ${RPM_BUILD_ROOT}/usr/local/bin
ln -s %{_prefix}/etc/SupplyFinanceChaind.cfg ${RPM_BUILD_ROOT}/etc/opt/SupplyFinanceChain/SupplyFinanceChaind.cfg
ln -s %{_prefix}/etc/validators.txt ${RPM_BUILD_ROOT}/etc/opt/SupplyFinanceChain/validators.txt
ln -s %{_prefix}/bin/SupplyFinanceChaind ${RPM_BUILD_ROOT}/usr/local/bin/SupplyFinanceChaind
install -D validator-keys-tool/bld.release/validator-keys ${RPM_BUILD_ROOT}%{_bindir}/validator-keys
install -D ./SupplyFinanceChaind/Builds/containers/shared/SupplyFinanceChaind.service ${RPM_BUILD_ROOT}/usr/lib/systemd/system/SupplyFinanceChaind.service
install -D ./SupplyFinanceChaind/Builds/containers/packaging/rpm/50-SupplyFinanceChaind.preset ${RPM_BUILD_ROOT}/usr/lib/systemd/system-preset/50-SupplyFinanceChaind.preset
install -D ./SupplyFinanceChaind/Builds/containers/shared/update-SupplyFinanceChaind.sh ${RPM_BUILD_ROOT}%{_bindir}/update-SupplyFinanceChaind.sh
install -D ./SupplyFinanceChaind/Builds/containers/shared/update-SupplyFinanceChaind-cron ${RPM_BUILD_ROOT}%{_prefix}/etc/update-SupplyFinanceChaind-cron
install -D ./SupplyFinanceChaind/Builds/containers/shared/SupplyFinanceChaind-logrotate ${RPM_BUILD_ROOT}/etc/logrotate.d/SupplyFinanceChaind
install -d $RPM_BUILD_ROOT/var/log/SupplyFinanceChaind
install -d $RPM_BUILD_ROOT/var/lib/SupplyFinanceChaind

%post
USER_NAME=SupplyFinanceChaind
GROUP_NAME=SupplyFinanceChaind

getent passwd $USER_NAME &>/dev/null || useradd $USER_NAME
getent group $GROUP_NAME &>/dev/null || groupadd $GROUP_NAME

chown -R $USER_NAME:$GROUP_NAME /var/log/SupplyFinanceChaind/
chown -R $USER_NAME:$GROUP_NAME /var/lib/SupplyFinanceChaind/
chown -R $USER_NAME:$GROUP_NAME %{_prefix}/

chmod 755 /var/log/SupplyFinanceChaind/
chmod 755 /var/lib/SupplyFinanceChaind/

chmod 644 %{_prefix}/etc/update-SupplyFinanceChaind-cron
chmod 644 /etc/logrotate.d/SupplyFinanceChaind
chown -R root:$GROUP_NAME %{_prefix}/etc/update-SupplyFinanceChaind-cron

%files
%doc SupplyFinanceChaind/README.md SupplyFinanceChaind/LICENSE
%{_bindir}/SupplyFinanceChaind
/usr/local/bin/SupplyFinanceChaind
%{_bindir}/update-SupplyFinanceChaind.sh
%{_prefix}/etc/update-SupplyFinanceChaind-cron
%{_bindir}/validator-keys
%config(noreplace) %{_prefix}/etc/SupplyFinanceChaind.cfg
%config(noreplace) /etc/opt/SupplyFinanceChain/SupplyFinanceChaind.cfg
%config(noreplace) %{_prefix}/etc/validators.txt
%config(noreplace) /etc/opt/SupplyFinanceChain/validators.txt
%config(noreplace) /etc/logrotate.d/SupplyFinanceChaind
%config(noreplace) /usr/lib/systemd/system/SupplyFinanceChaind.service
%config(noreplace) /usr/lib/systemd/system-preset/50-SupplyFinanceChaind.preset
%dir /var/log/SupplyFinanceChaind/
%dir /var/lib/SupplyFinanceChaind/

%files devel
%{_prefix}/include
%{_prefix}/lib/*.a
%{_prefix}/lib/cmake/SupplyFinanceChain

%changelog
* Wed May 15 2019 Mike Ellery <mellery451@gmail.com>
- Make validator-keys use local SupplyFinanceChaind build for core lib

* Wed Aug 01 2018 Mike Ellery <mellery451@gmail.com>
- add devel package for signing library

* Thu Jun 02 2016 Brandon Wilson <bwilson@SupplyFinanceChain.com>
- Install validators.txt

