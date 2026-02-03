#!/usr/bin/bash

if test ! -f ./install_vim.sh; then
	>&2 echo "You must run this script from the same folder it is in"
	exit
fi
echo "uninstalling from ~/.vim/"

rm ~/.vim/syntax/pup.vim &&
rm ~/.vim/ftdetect/pup.vim

echo "[33mdone uninstalling[0m"
