#!/bin/bash

# Lista kluczy IPC
KEYS=(
0x00001ab8
0x00001ab9
0x00001abc
0x00001abd
0x00001abe
0x00001abf
0x00001aba
0x00001abb
0x000011e4
0x000011e5
0x000011e6
0x000011e7
0x000011e8
0x000011e9
)

for key in "${KEYS[@]}"; do
    echo "Usuwanie zasobów dla klucza $key"

    # Message Queues
    ipcs -q | awk -v k="$key" '$1 == k {print $2}' | while read id; do
        echo "  usuwam msq id=$id"
        ipcrm -q "$id"
    done

    # Shared Memory
    ipcs -m | awk -v k="$key" '$1 == k {print $2}' | while read id; do
        echo "  usuwam shm id=$id"
        ipcrm -m "$id"
    done

    # Semaphores
    ipcs -s | awk -v k="$key" '$1 == k {print $2}' | while read id; do
        echo "  usuwam sem id=$id"
        ipcrm -s "$id"
    done
done

echo "Czyszczenie IPC zakończone."
