# Demo Script

1. Show the repository on GitHub: docs, src, tests, branches and commit history.
2. Build:
       bash scripts/gen_cert.sh
       cmake -S . -B build
       cmake --build build -j"$(nproc)"
3. Tab 1, start the server:
       ./build/sft server 9000
4. Tab 2, send a 5 MB file:
       head -c 5000000 /dev/urandom > /tmp/demo.bin
       ./build/sft client 9000 /tmp/demo.bin
   Point out: certificate verified, SHA-256 printed on both sides, Server replied OK.
5. Show the resume feature:
       rm -f received/demo.bin received/demo.bin.part
       SFT_STOP_AFTER=2000000 ./build/sft client 9000 /tmp/demo.bin
       ls -l received
       ./build/sft client 9000 /tmp/demo.bin
       cmp /tmp/demo.bin received/demo.bin && echo "FILES MATCH"
6. Run the automated tests:
       bash tests/run_tests.sh
