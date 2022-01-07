#!/bin/sh
#
# To install:
# cp soil-watcher.sh /etc/init.d/soil-watcher
#
# Program comes from https://github.com/embeddedTS/soil-watcher
#
# Remember to `make install` to copy binary to /usr/local/bin.
# Also remember to make this /etc/init.d/soil-watcher executable (chmod +x)
#
# To make this run during startup and shutdown:
#   update-rc.d soil-watcher defaults
#
# To manually start and stop the script:
#   /etc/init.d/soil-watcher start
#   /etc/init.d/soil-watcher stop

case "$1" in
  start)
    nohup /usr/local/bin/soil-watcher -d
    ;;
  stop)
    kill $(ps aux | grep '/usr/local/bin/[s]oil-watcher' | awk '{print $2}')
    ;;
  *)
    echo "Usage: soil-watcher start|stop"
    exit 3
    ;;
esac

exit 0
