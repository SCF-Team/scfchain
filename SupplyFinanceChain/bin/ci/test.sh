#!/usr/bin/env bash

set -o xtrace
set -o errexit

# Set to 'true' to run the known "manual" tests in SupplyFinanceChaind.
MANUAL_TESTS=${MANUAL_TESTS:-false}
# The maximum number of concurrent tests.
CONCURRENT_TESTS=${CONCURRENT_TESTS:-$(nproc)}
# The path to SupplyFinanceChaind.
SUPPLYFINANCECHAIND=${SUPPLYFINANCECHAIND:-build/SupplyFinanceChaind}
# Additional arguments to SupplyFinanceChaind.
SUPPLYFINANCECHAIND_ARGS=${SUPPLYFINANCECHAIND_ARGS:-}

function join_by { local IFS="$1"; shift; echo "$*"; }

declare -a manual_tests=(
  'beast.chrono.abstract_clock'
  'beast.unit_test.print'
  'SupplyFinanceChain.NodeStore.Timing'
  'SupplyFinanceChain.app.Flow_manual'
  'SupplyFinanceChain.app.NoSupplyFinanceChainCheckLimits'
  'SupplyFinanceChain.app.PayStrandAllPairs'
  'SupplyFinanceChain.consensus.ByzantineFailureSim'
  'SupplyFinanceChain.consensus.DistributedValidators'
  'SupplyFinanceChain.consensus.ScaleFreeSim'
  'SupplyFinanceChain.SupplyFinanceChain_data.digest'
  'SupplyFinanceChain.tx.CrossingLimits'
  'SupplyFinanceChain.tx.FindOversizeCross'
  'SupplyFinanceChain.tx.Offer_manual'
  'SupplyFinanceChain.tx.OversizeMeta'
  'SupplyFinanceChain.tx.PlumpBook'
)

if [[ ${MANUAL_TESTS} == 'true' ]]; then
  SUPPLYFINANCECHAIND_ARGS+=" --unittest=$(join_by , "${manual_tests[@]}")"
else
  SUPPLYFINANCECHAIND_ARGS+=" --unittest --quiet --unittest-log"
fi
SUPPLYFINANCECHAIND_ARGS+=" --unittest-jobs ${CONCURRENT_TESTS}"

${SUPPLYFINANCECHAIND} ${SUPPLYFINANCECHAIND_ARGS}
