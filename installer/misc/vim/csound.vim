" Vim syntax files for Csound
" Maintainer:   Istvan Varga
" Last change:  Mar 2 2006

augroup	filetypedetect
au	BufNewFile,BufRead	*.orc	setlocal filetype=csound_orc
au	BufNewFile,BufRead	*.ORC	setlocal filetype=csound_orc
au	BufNewFile,BufRead	*.sco	setlocal filetype=csound_sco
au	BufNewFile,BufRead	*.SCO	setlocal filetype=csound_sco
au	BufNewFile,BufRead	*.csd	setlocal filetype=csound_csd
au	BufNewFile,BufRead	*.CSD	setlocal filetype=csound_csd
augroup	END

