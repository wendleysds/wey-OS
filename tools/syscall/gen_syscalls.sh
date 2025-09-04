#!/bin/bash
# Generate a syscall table header.
# Each line of the syscall table should have the following format:
#
# NR ABI NAME ENTRY
#
# NR       syscall number
# ABI      ABI name
# NAME     syscall name
# ENTRY    entry point

set -e

usage() {
	echo >&2 "usage: $0 infile.tbl outfile.h" >&2
	echo >&2
	exit 1
}

if [ $# -ne 2 ]; then
	usage
fi

infile="$1"
outfile="$2"

nxt=0

grep -E "^[0-9]+[[:space:]]" "$infile" | {
	echo "// AUTO GENERATED!"
	while read nr abi name entry; do

		if [ $nxt -gt $nr ]; then
			echo "error: $infile: syscall table is not sorted or have duplicates!" >&2
			exit 1
		fi

		while [ $nxt -lt $nr ]; do
			nxt=$((nxt + 1))
		done

		if [ -n "$entry" ]; then
			echo "__SYSCALL($nr, $entry)"
		fi

		nxt=$((nr + 1))
	done
} > "$outfile"