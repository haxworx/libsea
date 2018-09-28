#!/bin/sh
ecrustify -l C -c fmt/kr.cfg --no-backup "$@"
