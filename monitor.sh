#!/bin/sh
nodemon -e cpp,hpp -i bin -x bash -c "make -j16 && ./bin/main"
