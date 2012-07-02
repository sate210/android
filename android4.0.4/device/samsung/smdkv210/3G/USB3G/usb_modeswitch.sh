#!/sbin/busybox ash
sleep 5
#/system/bin/setprop "mode_switch_running" "1"

if [ -f $1 ]; then
  echo 1 >/sys/class/sec/switch/usb3g
	/system/bin/usb_modeswitch -W -I -c $1
else
	echo "$1 does not exist."
fi

#/system/bin/setprop "mode_switch_running" "0"
