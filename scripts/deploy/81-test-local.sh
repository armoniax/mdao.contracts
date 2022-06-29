
merchant=$1
token="cnyd.token"
contract=${2}.o
confContract=${2}.0


cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog
merchant=joss1
token="cnyd.token"
contract=deotcr.o
order_id=0
deal_id=0
user=chenjunqiang
merchant_t=2
user_t=3

#10、商户充值押金
cleos push action ${token} transfer '['${merchant}', "'${contract}'", "100.000000 CNYD", "deposit"]' -p ${merchant}

#open order
cleos push action ${contract} openorder '['${merchant}', "buy", ["wechat","alipay"],"10.000000 CNYDARC", "1.1000 CNY", "1.000000 CNYDARC", "3.000000 CNYDARC","quick conform 1.1"]' -p ${merchant}

cleos push action ${contract} openorder '['${merchant}', "sell",  ["wechat"],"20.000000 CNYDARC", "1.0000 CNY", "0.500000 CNYDARC", "3.000000 CNYDARC", "quick conform"]' -p ${merchant}@active

cleos get table ${contract} ${contract} sellorders -l 100

#暂停订单
cleos push action ${contract} pauseorder '['${merchant}', "buy",'${order_id}' ]' -p ${merchant}
cleos push action ${contract} resumeorder '['${merchant}', "buy",'${order_id}' ]' -p ${merchant}
cleos push action ${contract} closeorder '['${merchant}', "buy",'${order_id}' ]' -p ${merchant}


#用户吃单
cleos push action ${contract} opendeal '['${user}', "buy",'${order_id}', "1.000000 CNYDARC", 1023, "pay_type:wechat","user_ss","merchant_ss","session_msg"]' -p ${user}@active
cleos push action ${contract} opendeal '['${user}', "sell",'${order_id}', "0.500000 CNYDARC", 2, "pay_type:wechat","user_ss","merchant_ss","session_msg"]' -p ${user}@active

cleos get table ${contract} ${contract} deals

#商户确认订单 1
cleos push action ${contract} processdeal '['${merchant}', '${merchant_t}', '${deal_id}', 2, "pey_type:wechant:abcxxx"]' -p ${merchant}
#客户确认订单 1
cleos push action ${contract} processdeal '['${user}', '${user_t}', '${deal_id}', 3, "pey_type:wechant:abcxxx"]' -p ${user}


####仲裁#####
cleos push action ${contract} startarbit  '['${merchant}', '${merchant_t}', '${deal_id}', "amaxhu3t3tjd", "arbit_ss", "session_msg"]' -p ${merchant}

cleos push action ${contract} closearbit  '["amaxhu3t3tjd", '${deal_id}', 1,"arbit_session_msg"]' -p "amaxhu3t3tjd"


#### 添加CNYD
cleos push action ${token} transfer '["eosio", "user3", "1000.000000 CNYD", ""]' -p eosio@active
### 添加AMA
cleos push action ${token} transfer '["eosio", "user3", "1000.0000 AMA", ""]' -p eosio@active

#激活
cleos create account eosio ${user} ${pubKey} -p eosio@active

STAKE_NET='1.0000 AMA'
STAKE_CPU='1.0000 AMA'
cleos system newaccount eosio ${user} ${pubKey} ${pubKey} --stake-net "${STAKE_NET}" --stake-cpu "${STAKE_CPU}" --buy-ram-kbytes 1800