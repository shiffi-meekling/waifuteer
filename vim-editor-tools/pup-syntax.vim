" Vim syntax file
" Language: waifuteer puppet config
" Maintainer: Shiffi Meekling
" Latest Revision: 03 Sept 2025

if exists("b:current_syntax")
  echo "failure"
  finish
endif

syntax match PupHeader	"===.*===" display
syntax match PupSectionType	"\w\+:"me=e-1 contained
syntax match PupSection	/\[.*\]/ display
		\ contains=PupSectionType,PupArrayAccess
syntax match PupSubSection	/^\s*\w\+:/ display
syntax match PupComment	"#.*" display
syntax match PupInbuiltVar	"\<_\w\+\>" contained
syntax match PupValues	"=[^#\n]*"hs=s+1 contained contains=PupInbuiltVar
syntax match PupFields	"[^#=]\+\(\s*=\)\@=" display contained
syntax match PupPair	"[^#=]\+\s*=[^#\n]*" contains=PupValues,PupFields
syntax match PupArray	"\w\+\[[^]\#\n]\+\]" display
syntax match PupArrayAccess	/\[[^][:]\+\]/ display transparent
		\ contained contains=PupArrayIndex
syntax match PupArrayIndex	/\w\+/ display contained 

highlight def PupHeader	cterm=bold ctermfg=yellow ctermbg=DarkBlue term=bold 
highlight def PupSection	ctermfg=Red cterm=bold
highlight def PupSubSection	ctermfg=Red 
" highlight def PupArrayAccess	ctermfg=Blue 
highlight def link PupComment	Comment
highlight def link PupValues	Constant
highlight def link PupFields	Label
highlight def link PupSectionType	Type
highlight def link PupInbuiltVar	Identifier
highlight def link PupArray	Identifier
highlight def PupArrayIndex	ctermfg=Red cterm=NONE term=NONE

let b:current_syntax = "pup"
" vim:ts=18
