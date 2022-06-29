  acct=rewardnewbie
  contract=rewardnewbie

  echo "# Deploy contract: $contract"
  tcli set contract $acct ./build/contracts/$contract -p $acct@active