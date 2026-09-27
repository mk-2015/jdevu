#!/usr/bin/env bash

rm -f *.o *.ko *.mod *.mod.c *.order Module.symvers .*.cmd .*.o.d .tmp_versions/
find . -name '*.o' -o -name '*.ko' -o -name '.*.cmd' -o -name '*.mod.c' -o -name '*.mod' -exec rm -f {} +