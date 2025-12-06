MANUAL_UUID=16ab5365-9fd3-4473-aa16-c67c8a3dc120
AUTO_UUID=7f92582e-c367-4c6d-be6f-8a9e37db64a1

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

