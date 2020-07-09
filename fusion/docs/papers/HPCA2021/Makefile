RM     := /bin/rm -rf

all: 
	latex main ; bibtex main; latex main; latex main; dvips  -G0 -o paper.ps -e 0 main; ps2pdf paper.ps &

nobib: 
	latex main ; latex main; latex main; dvips  -G0 -o paper.ps -e 0 main; pst2pdf14 paper.ps

clean: 
	$(RM) *.dvi paper.ps *.log *.aux *.pdf *.blg *.bbl *~* main.out

