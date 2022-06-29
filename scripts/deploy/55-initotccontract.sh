
initDeotcContract() {
  updateContract="cleos push action ${otcAccountName} init \"[ '${confAccountName}']\" -p  ${otcAccountName}@active"
  echo "-------initContract--------------"
  echo ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
  ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
}

remoteDockerScrip='docker exec -i amax-devnet /bin/bash -c'
otcAccountName=${1}.o
confAccountName=${1}.h

sh ./scripts/deploy/01-unlock.sh
initDeotcContract