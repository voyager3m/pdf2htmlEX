
rm -f *.{html,css,xml,js,png,woff,template}

#./pdf2htmlEX/build/pdf2htmlEX  \
#        --embed-css=0 --embed-font=0 \
#        --embed-image=0 --embed-javascript=0 --embed-outline=0 --zoom=1.6 --css-filename=styles.css \
#        --outline-filename=outline.xml --split-pages=1 \
#        --fallback=0 --page-filename="out.html" $1 page.template



#./run_pdf2htmlex.sh  \
#        --embed-css=0 --embed-font=0 \
#        --embed-image=0 --embed-javascript=0 --embed-outline=0 --zoom=1.6 --css-filename=styles.css \
#        --outline-filename=outline.xml --split-pages=1 \
#        --fallback=0 --page-filename="out.html" $1 page.template



./run_pdf2htmlex.sh --zoom=1.6  $1
#./pdf2htmlEX/build/pdf2htmlEX  --zoom=1.6  $1
#./pdf2htmlEX/build/pdf2htmlEX  ./h1-h6.pdf

