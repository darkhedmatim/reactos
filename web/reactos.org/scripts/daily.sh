#!/bin/sh

cd /web/reactos.org/scripts

./update_source.sh
cd ../source/reactos
perl ../../scripts/RosResCheck.pl > ../../htdocs/generated/transstatus.html
