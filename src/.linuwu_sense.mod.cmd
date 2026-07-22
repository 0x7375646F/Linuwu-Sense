savedcmd_src/linuwu_sense.mod := printf '%s\n'   src/linuwu_sense.o | awk '!x[$$0]++ { print("./"$$0) }' > src/linuwu_sense.mod
