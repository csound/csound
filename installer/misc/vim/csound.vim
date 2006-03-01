" Vim syntax files for Csound
" Maintainer:   Istvan Varga
" Last change:  Mar 1 2006

augroup	filetypedetect
au	BufNewFile,BufRead	*.orc	setf csound_orc
au	BufNewFile,BufRead	*.ORC	setf csound_orc
au	BufNewFile,BufRead	*.sco	setf csound_sco
au	BufNewFile,BufRead	*.SCO	setf csound_sco
au	BufNewFile,BufRead	*.csd	setf csound_csd
au	BufNewFile,BufRead	*.CSD	setf csound_csd
augroup	END

