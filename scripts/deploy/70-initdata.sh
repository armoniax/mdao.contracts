initData() {

  importPrivkeyShell="cleos wallet import --private-key ${privKey}"
  echo "-------import priv key--------------"
  ssh sh-misc "${remoteDockerScrip} '${importPrivkeyShell}'"

  #设置商户
  setMerchantShell="cleos push action ${otcAccountName} setmerchant  \"[ '${merchant}' , '${merchant}' , '${merchant}' , 'xxxama.com', 'dddddddd']\" -p ${merchant}@active"
  ssh sh-misc "${remoteDockerScrip} '${setMerchantShell}'"
}

sh ./scripts/deploy/01-unlock.sh
otcAccountName=${1}.o
remoteDockerScrip='docker exec -i amax-devnet /bin/bash -c'

array=("5KJyakfLhb1JGcLPqx9sFoWNoeBGvSk9NBTBCY45rHVXXitGqnT:joss1"
        "5JZNkpLZdb7USfCea44wBsqPsm8us2bDKDXCmGjiDJ96o6pHtjt:chenjunqiang"
        "5K5T8764xLVM5eRCAwNhLKzz7Bys4JDNaKrLU5tbRkD2EtLLnTe:amaxhu3t3tjd"
        "5KVsZcENtALWsedrC1k4jnbXuDiNJqQmshgLqpteG5fHJqiQgKA:jason1")
for i in "${array[@]}"; do   # The quotes are necessary here
    IFS=':' read -r -a arr <<< "$i"
    privKey=${arr[0]}
    merchant=${arr[1]}
    initData
done