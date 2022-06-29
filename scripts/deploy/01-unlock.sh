unlock() {
  unlock="cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog"
  ssh sh-misc "${remoteDockerScrip} '${unlock}'"
}
remoteDockerScrip='docker exec -i amax-devnet /bin/bash -c'
unlock