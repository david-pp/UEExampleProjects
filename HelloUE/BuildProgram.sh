#!/bin/sh

source ~/.bash_profile
ubt $1 Mac Debug -Project=$PWD/HelloUE.uproject
#ubt $1 Mac Development -Project=$PWD/HelloUE.uproject