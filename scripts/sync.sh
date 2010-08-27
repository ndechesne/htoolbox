#!/bin/sh -e
if [ -d /backup/scripts ]; then
	rsync --archive --delete --exclude lost+found /backup/ /archive
else
	echo "Backup not mounted!?!" >&2
fi
