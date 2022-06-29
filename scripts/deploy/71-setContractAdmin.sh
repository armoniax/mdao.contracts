initContractAdmin() {

  #设置合约
  setMerchantShell="cleos push action ${otcAccountName} setadmin  \"[ 'admin' ]\" -p ${otcAccountName}@active"
  ssh sh-misc "${remoteDockerScrip} '${setMerchantShell}'"
}

sh ./scripts/deploy/01-unlock.sh
otcAccountName=${1}.o
remoteDockerScrip='docker exec -i amax-devnet /bin/bash -c'

initContractAdmin