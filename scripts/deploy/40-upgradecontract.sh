updateContract(){
  contractFilePath='/opt/amax/node_devnet/data/otccontract'
  updateContract="cleos set contract ${accountName} ${contractFilePath} ${otcFileName}.wasm ${otcFileName}.abi -p ${accountName}@active"
  echo   ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
  ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
}

sh ./scripts/deploy/01-unlock.sh

remoteDockerScrip='docker exec -i amax-devnet /bin/bash -c'
accountName=${1}.o
otcFileName='otcbook'
updateContract 

accountName=${1}.h
otcFileName='otcconf'
# updateContract 