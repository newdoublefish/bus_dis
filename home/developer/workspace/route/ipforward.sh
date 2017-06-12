echo 1 >/proc/sys/net/ipv4/ip_forward
iptables -t nat -A POSTROUTING -s 192.168.0.0/24 -o wlp1s0 -j MASQUERADE
iptables -A FORWARD -s 192.168.0.0/24 -o wlp1s0 -j ACCEPT
iptables -A FORWARD -d 192.168.0.0/24 -m conntrack --ctstate ESTABLISHED,RELATED -i wlp1s0 -j ACCEPT

