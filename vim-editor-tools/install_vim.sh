#!/usr/bin/bash

if test ! -f ./install_vim.sh; then
	>&2 echo "You must run this script from the same folder it is in"
	exit
fi
echo "installing into ~/.vim/"

mkdir -p ~/.vim/syntax/ &&
cp pup-syntax.vim ~/.vim/syntax/pup.vim &&
mkdir -p ~/.vim/ftdetect/ &&
cp pup-ftdetect.vim ~/.vim/ftdetect/pup.vim

head ~/.vim/syntax/pup.vim ~/.vim/ftdetect/pup.vim
echo "[33mdone installing[0m"
