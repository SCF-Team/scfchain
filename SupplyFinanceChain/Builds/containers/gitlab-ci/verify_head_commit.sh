#!/usr/bin/env sh
set -ex
apt -y update
apt -y install software-properties-common curl git gnupg
curl -sk -o SupplyFinanceChaind-pubkeys.txt "${GIT_SIGN_PUBKEYS_URL}"
gpg --import SupplyFinanceChaind-pubkeys.txt
if git verify-commit HEAD; then
    echo "git commit signature check passed"
else
    echo "git commit signature check failed"
    git log -n 5 --color
        --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an> [%G?]%Creset'
        --abbrev-commit
    exit 1
fi

