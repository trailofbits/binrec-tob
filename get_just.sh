#!/bin/bash

sudo apt-get install -y curl
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | sudo bash -s -- --to /usr/local/bin

just --completions bash > ~/.just_completion
chmod 755 ~/.just_completion


cat << EOF >> ~/.bashrc

if [ -f ~/.just_completion ]; then
    . ~/.just_completion
fi
EOF