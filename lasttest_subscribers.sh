#!/bin/bash 

# Zuerst alte Prozesse killen
killall mosquitto_sub
killall mqtt_server

# Server starten
./mqtt_server &

sleep 1

# Subscriber starten
mosquitto_sub -i s00 -t "t/#" -h 192.168.0.2 -p 1883 > lasttest_subscriber_messages &

sleep 1

# 500 Nachrichten generieren
for i in `seq 1 100`; do 
 mosquitto_pub -i c00 -t "t/0" -h 192.168.0.2 -p 1883 -m 1.000000
 mosquitto_pub -i c01 -t "t/1" -h 192.168.0.2 -p 1883 -m 1.000000
 mosquitto_pub -i c02 -t "t/2" -h 192.168.0.2 -p 1883 -m 1.000000
 mosquitto_pub -i c03 -t "t/3" -h 192.168.0.2 -p 1883 -m 1.000000
 mosquitto_pub -i c04 -t "t/4" -h 192.168.0.2 -p 1883 -m 1.000000
done

sleep 1

killall mosquitto_sub
killall mqtt_server

wc -l lasttest_subscriber_messages