initData() {

  setMerchantShell="cleos push action ${otcAccountName} setmerchant  \"[ '${merchant}' , '${merchant}' , '${merchant}' , 'xxxama.com', 'dddddddd']\" -p ${merchant}@active"
  ssh sh-misc "${remoteDockerScrip} '${setMerchantShell}'"
}


sh ./scripts/deploy/01-unlock.sh
otcAccountName=${1}.o
remoteDockerScrip='docker exec -i amax-devnet /bin/bash -c'