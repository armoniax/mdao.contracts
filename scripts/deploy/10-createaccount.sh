# create key
createKeyAndImport() {
    unlockScript='cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog'
    ssh sh-misc "${remoteDockerScrip} '${unlockScript}'"
    createAccountScript='cleos create key --to-console'
    ret=`ssh sh-misc "${remoteDockerScrip} '${createAccountScript}'"`
    echo "create key: $ret"
    privKey=${ret:13:51}
    pubKey=`echo $ret | sed -n '1p'`
    pubKey=${pubKey:0-54:54}
    
    importPriKeyScript="cleos wallet import --private-key ${privKey}"
    ret=`ssh sh-misc "${remoteDockerScrip} '${importPriKeyScript}'"`
    echo "${ret}"

}

#newAccountAndActive
newAccountAndActive(){
  contractName=$1
  STAKE_NET='1.0000 AMA'
  STAKE_CPU='1.0000 AMA'
  newAccountAndActiveScript="cleos system newaccount eosio ${contractName} ${pubKey} ${pubKey} --stake-net \"${STAKE_NET}\" --stake-cpu \"${STAKE_CPU}\" --buy-ram-kbytes 1800"
  ret=`ssh sh-misc "${remoteDockerScrip} '${newAccountAndActiveScript}' "`
  echo "newAccountAndActive output: $ret"
}


# eg: deotc.2a
accountName=${1}.o
remoteDockerScrip='docker exec -i amax-devnet /bin/bash -c'

##create account
echo "--------------------------        createKey 函数开始执行            --------------------------"
createKeyAndImport
echo "--------------------------        createKey 函数结束执行            --------------------------"

##newAccountAndActive
echo "--------------------------    newAccountAndActive  函数开始执行     --------------------------"
newAccountAndActive "$accountName"
echo "--------------------------    newAccountAndActive  函数结束执行     --------------------------"


# eg: deotc.2a
accountName=${1}.h
remoteDockerScrip='docker exec -i amax-devnet /bin/bash -c'

##create account
echo "--------------------------        createKey 函数开始执行            --------------------------"
createKeyAndImport
echo "--------------------------        createKey 函数结束执行            --------------------------"

##newAccountAndActive
echo "--------------------------    newAccountAndActive  函数开始执行     --------------------------"
newAccountAndActive "$accountName"
echo "--------------------------    newAccountAndActive  函数结束执行     --------------------------"

