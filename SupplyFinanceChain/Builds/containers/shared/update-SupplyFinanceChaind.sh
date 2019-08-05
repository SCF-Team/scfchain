#!/usr/bin/env bash

# auto-update script for SupplyFinanceChaind daemon

LOCKDIR=/tmp/SupplyFinanceChainupdate.lock
UPDATELOG=/var/log/SupplyFinanceChaind/update.log

function cleanup {
  # If this directory isn't removed, future updates will fail.
  rmdir $LOCKDIR
}

# Use mkdir to check if process is already running. mkdir is atomic, as against file create.
if ! mkdir $LOCKDIR 2>/dev/null; then
  echo $(date -u) "lockdir exists - won't proceed." >> $UPDATELOG
  exit 1
fi
trap cleanup EXIT

source /etc/os-release
can_update=false

if [[ "$ID" == "ubuntu" || "$ID" == "debian" ]] ; then
  # Silent update
  apt-get update -qq

  # The next line is an "awk"ward way to check if the package needs to be updated.
  SUPPLYFINANCECHAIN=$(apt-get install -s --only-upgrade SupplyFinanceChaind | awk '/^Inst/ { print $2 }')
  test "$SUPPLYFINANCECHAIN" == "SupplyFinanceChaind" && can_update=true

  function apply_update {
    apt-get install SupplyFinanceChaind -qq
  }
elif [[ "$ID" == "fedora" || "$ID" == "centos" || "$ID" == "rhel" || "$ID" == "scientific" ]] ; then
  SUPPLYFINANCECHAIN_REPO=${SUPPLYFINANCECHAIN_REPO-stable}
  yum --disablerepo=* --enablerepo=SupplyFinanceChain-$SUPPLYFINANCECHAIN_REPO clean expire-cache

  yum check-update -q --enablerepo=SupplyFinanceChain-$SUPPLYFINANCECHAIN_REPO SupplyFinanceChaind || can_update=true

  function apply_update {
    yum update -y --enablerepo=SupplyFinanceChain-$SUPPLYFINANCECHAIN_REPO SupplyFinanceChaind
  }
else
  echo "unrecognized distro!"
  exit 1
fi

# Do the actual update and restart the service after reloading systemctl daemon.
if [ "$can_update" = true ] ; then
  exec 3>&1 1>>${UPDATELOG} 2>&1
  set -e
  apply_update
  systemctl daemon-reload
  systemctl restart SupplyFinanceChaind.service
  echo $(date -u) "SupplyFinanceChaind daemon updated."
else
  echo $(date -u) "no updates available" >> $UPDATELOG
fi

