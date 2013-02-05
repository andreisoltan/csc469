docs: report.pdf

report.pdf: report.tex
	latex report.tex
	latex report.tex
	dvips -o report.ps report.dvi
	ps2pdf report.ps

clean:
	rm -rf report.aux report.dvi report.log report.pdf report.ps report.toc
