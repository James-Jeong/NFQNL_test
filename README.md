# NFQNL_test

# Commands
1. iptables -A INPUT -p tcp --sport 80 -j NFQUEUE --queue-num 0
2. iptables -A OUTPUT -p tcp --dport 80 -j NFQUEUE --queue-num 0
3. qmake
4. make
5. sudo ./NFQNL

# Environment
1. Ubuntu 18.04
2. Qt creator 4.5.2
