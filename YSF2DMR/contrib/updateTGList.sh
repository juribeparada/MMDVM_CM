wget -O /tmp/group.txt http://master.brandmeister.es/status/status.php
wget -O /tmp/data.json http://api.brandmeister.network/v1.0/groups/
/usr/bin/python /usr/local/bin/tg_generate.py
mount -o remount,rw /
rm /usr/local/etc/TGList.txt
mv /tmp/TGList.txt /usr/local/etc
mount -o remount,ro /
exit 0
