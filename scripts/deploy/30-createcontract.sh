# create contract
# param1: file name
createContract() {
    otcFileName=$1
    accountName=$2
 
    unlockScript='cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog'
    ssh sh-misc "${remoteDockerScrip} '${unlockScript}'"

    contractFilePath='/opt/amax/node_devnet/data/otccontract'
    setContractScript="cleos set contract ${accountName} ${contractFilePath} ${otcFileName}.wasm ${otcFileName}.abi -p ${accountName}@active"
    ssh sh-misc "${remoteDockerScrip} '${setContractScript}'"
}


accountName=${1}.h
remoteDockerScrip='docker exec -i amax-devnet /bin/bash -c'
otcFileName='otcconf'
createContract ${otcFileName} ${accountName}

accountName=${1}.o
otcFileName='otcbook'
createContract ${otcFileName} ${accountName}

