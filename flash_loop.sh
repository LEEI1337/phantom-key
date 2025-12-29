#!/bin/bash
echo "=========================================="
echo "BOOT-MODUS PROCEDURE:"
echo "1. BOOT halten"
echo "2. RESET drücken"
echo "3. BOOT loslassen"
echo ""
echo "Versuche zu flashen (60 Sekunden)..."
echo "=========================================="

source /tmp/esp-venv/bin/activate

for i in $(seq 1 30); do
    echo -n "Versuch $i: "
    if python3 -m esptool --chip esp32c3 --port /dev/ttyACM0 --baud 921600 --before no_reset write_flash 0x0 .pio/build/esp32c3/bootloader.bin 0x8000 .pio/build/esp32c3/partitions.bin 0x10000 .pio/build/esp32c3/firmware.bin 2>&1 | grep -q "Hash of data verified"; then
        echo "ERFOLG!"
        deactivate
        exit 0
    else
        echo "Warte auf Boot-Modus..."
    fi
    sleep 2
done

deactivate
echo ""
echo "FEHLER: Konnte nicht flashen."
echo "Bitte prüfe ob ESP32 einen BOOT-Knopf hat."
