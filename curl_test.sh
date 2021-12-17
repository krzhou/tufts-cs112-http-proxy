#!/usr/bin/bash

proxy='localhost:9999'


#### Demo cache ####

# # Test with small HTTP page.
# echo '================'
# name='bio'
# url='http://www.cs.cmu.edu/~prs/bio.html'
# echo "GET ${url}"
# echo 'direct access'
# time \
# curl "${url}" \
# --get --verbose --include \
# --output ${name}-response-direct.txt \
# --stderr ${name}-verbose-direct.txt
# echo
# echo 'uncached proxy access'
# time \
# curl "${url}" \
# --get --verbose --include \
# --proxy ${proxy} \
# --output ${name}-response-uncached.txt \
# --stderr ${name}-verbose-uncached.txt
# echo
# echo 'cached proxy access'
# time \
# curl "${url}" \
# --get --verbose --include \
# --proxy ${proxy} \
# --output ${name}-response-cached.txt \
# --stderr ${name}-verbose-cached.txt
# echo
# # Compare direct and proxy access.
# echo 'compare responses with header'
# diff ${name}-response-direct.txt ${name}-response-cached.txt
# echo
# echo '================'


# # Test with large HTTP page.
# echo '================'
# name='jpg'
# url='http://www.cs.cmu.edu/~dga/dga-headshot.jpg'
# echo "GET ${url}"
# echo 'direct access'
# time \
# curl "${url}" \
# --get --verbose --include \
# --output ${name}-response-direct.txt \
# --stderr ${name}-verbose-direct.txt
# echo
# echo 'uncached proxy access'
# time \
# curl "${url}" \
# --get --verbose --include \
# --proxy ${proxy} \
# --output ${name}-response-uncached.txt \
# --stderr ${name}-verbose-uncached.txt
# echo
# echo 'cached proxy access'
# time \
# curl "${url}" \
# --get --verbose --include \
# --proxy ${proxy} \
# --output ${name}-response-cached.txt \
# --stderr ${name}-verbose-cached.txt
# echo
# # Compare direct and proxy access.
# echo 'compare responses with header'
# diff ${name}-response-direct.txt ${name}-response-cached.txt
# echo
# echo '================'


# # Test with HTTPS page.
# echo '================'
# name='google'
# url='https://www.google.com'
# echo "CONNECT ${url}"
# echo 'direct access'
# time \
# curl "${url}" \
# --get --insecure --verbose --include \
# --output ${name}-response-direct.txt \
# --stderr ${name}-verbose-direct.txt
# echo
# echo 'uncached proxy access'
# time \
# curl "${url}" \
# --get --insecure --verbose --include \
# --proxy ${proxy} \
# --output ${name}-response-uncached.txt \
# --stderr ${name}-verbose-uncached.txt
# echo
# echo 'cached proxy access'
# time \
# curl "${url}" \
# --get --insecure --verbose --include \
# --proxy ${proxy} \
# --output ${name}-response-cached.txt \
# --stderr ${name}-verbose-cached.txt
# echo
# echo '================'


# #### Chunked Encoding ####
# echo '================'
# name='scp'
# url='http://scp-wiki-cn.wikidot.com/'
# echo "GET ${url}"
# echo 'direct access'
# time \
# curl "${url}" \
# --get --verbose --include \
# --output ${name}-response-direct.txt \
# --stderr ${name}-verbose-direct.txt
# echo
# echo 'uncached proxy access'
# time \
# curl "${url}" \
# --get --verbose --include \
# --proxy ${proxy} \
# --output ${name}-response-uncached.txt \
# --stderr ${name}-verbose-uncached.txt
# echo
# echo 'cached proxy access'
# time \
# curl "${url}" \
# --get --verbose --include \
# --proxy ${proxy} \
# --output ${name}-response-cached.txt \
# --stderr ${name}-verbose-cached.txt
# echo
# # Compare direct and proxy access.
# echo 'compare responses with header'
# diff ${name}-response-direct.txt ${name}-response-cached.txt
# echo
# echo '================'
