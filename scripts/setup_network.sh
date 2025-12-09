MANUAL_UUID=bb010b29-b2df-413f-bf23-d6671c5ae5c1
AUTO_UUID=43e30262-a5df-4d7c-9fd5-217dbb7b2cdd

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

