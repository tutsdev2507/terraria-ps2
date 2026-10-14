#!/data/data/com.termux/files/usr/bin/sh
# Termux: pkg install clang make ; sh run_android.sh  -> abra http://localhost:8080 no Chrome
cd "$(dirname "$0")" && make -f Makefile.host terraria_web && ./terraria_web data 8080
