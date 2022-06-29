
#scp
scpToMiscMerchine(){
    remoteContractPath=/mnt/data/amax-test/eosio-docker/node_devnet/data/otccontract
    scp build/contracts/otcbook/otcbook* sh-misc:${remoteContractPath}
    scp build/contracts/otcconf/otcconf* sh-misc:${remoteContractPath}

}
scpToMiscMerchine