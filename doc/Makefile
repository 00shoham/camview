DOC=\
  cam-view.pdf\

INCLUDES=\
  config.tex\
  doc-header.tex\
  intro.tex\
  install.tex\
  tuning.tex\
  utils.tex\
  Makefile\

all: $(DOC)


%.pdf: %.tex $(INCLUDES)
	pdflatex $<
	pdflatex $<

clean: NULL
	rm -f $(DOC) *.aux *.log *.lot *.toc

NULL:
