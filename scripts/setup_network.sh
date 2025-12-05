MANUAL_UUID=1824d2f3-49fa-4b0e-b63f-d62924962dd2
AUTO_UUID=e23a2a54-dc1d-4342-8232-fe9b142d1044

nmcli connection modify $MANUAL_UUID ipv4.routing-rules ""
nmcli connection modify $MANUAL_UUID ipv4.route-table 200
nmcli connection modify $MANUAL_UUID +ipv4.routing-rules "priority 100 from 192.168.1.10 table 200"
nmcli connection modify $MANUAL_UUID ipv4.never-default yes
nmcli connection modify $MANUAL_UUID ipv4.method manual ipv4.addresses 192.168.1.10/24 ipv4.gateway 192.168.1.1
nmcli connection down $MANUAL_UUID
nmcli connection up $MANUAL_UUID

nmcli connection modify $AUTO_UUID ipv4.routing-rules ""
nmcli connection modify $AUTO_UUID ipv4.route-table 201
nmcli connection modify $AUTO_UUID +ipv4.routing-rules "priority 101 from 192.168.1.20 table 201"
nmcli connection modify $AUTO_UUID ipv4.never-default yes
nmcli connection modify $AUTO_UUID ipv4.method manual ipv4.addresses 192.168.1.20/24 ipv4.gateway 192.168.1.1
nmcli connection down $AUTO_UUID
nmcli connection up $AUTO_UUID

ip route get 192.168.1.1 from 192.168.1.10
ip route get 192.168.1.1 from 192.168.1.20

