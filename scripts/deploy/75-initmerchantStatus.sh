initData() {
  
  #设置商户状态
  echo "set merchant status ${merchant}"
  setMerchantStatusShell="cleos push action ${otcAccountName} enbmerchant  \"[ '${merchant}' , true ]\" -p admin"
  ssh sh-misc "${remoteDockerScript} '${setMerchantStatusShell}'"
}

deposit() {
  echo "merchant ${merchant} dipsoit 1000 CNYD"
  depositShell="cleos push action ${token} transfer \"['${merchant}', '${otcAccountName}', '1000.0000 CNYD', 'deposit']\" -p ${merchant}"
  echo $depositShell
  ssh sh-misc "${remoteDockerScript} '${depositShell}'"

}

sh ./scripts/deploy/01-unlock.sh
otcAccountName=${1}.o
token="eosio.token"

remoteDockerScript='docker exec -i amax-devnet /bin/bash -c'

array=("joss1"
        "chenjunqiang"
        "amaxhu3t3tjd"
        "jason1")
for i in "${array[@]}"; do   # The quotes are necessary here
    merchant=$i
    initData
    #deposit
done