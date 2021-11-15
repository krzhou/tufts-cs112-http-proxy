#!/usr/bin/bash

proxy='localhost:9160'

# Test with cs112 homepage.
name='cs112'
url='http://www.cs.tufts.edu/comp/112/index.html'
echo "GET ${url}"
# Direct access with response header.
curl "${url}" \
--get --verbose --include \
--output ${name}-response.txt \
--stderr ${name}-verbose.txt
# Direct access with no response header.
curl "${url}" \
--get --verbose \
--output ${name}-response-no-header.txt \
--stderr ${name}-verbose-no-header.txt
# Proxy access with response header.
curl "${url}" \
--get --verbose --include \
--proxy ${proxy} \
--output ${name}-response-proxy.txt \
--stderr ${name}-verbose-proxy.txt
# Proxy access with no response header.
curl "${url}" \
--get --verbose \
--proxy ${proxy} \
--output ${name}-response-proxy-no-header.txt \
--stderr ${name}-verbose-proxy-no-header.txt
# Compare direct and proxy access.
echo '- compare with response header'
diff ${name}-response.txt ${name}-response-proxy.txt
echo '- compare with no response header'
diff ${name}-response-no-header.txt ${name}-response-proxy-no-header.txt
echo

# Test with cached cs112 homepage.
name='cached-cs112'
url='http://www.cs.tufts.edu/comp/112/index.html'
echo "cached GET ${url}"
# Direct access with response header.
curl "${url}" \
--get --verbose --include \
--output ${name}-response.txt \
--stderr ${name}-verbose.txt
# Direct access with no response header.
curl "${url}" \
--get --verbose \
--output ${name}-response-no-header.txt \
--stderr ${name}-verbose-no-header.txt
# Proxy access with response header.
curl "${url}" \
--get --verbose --include \
--proxy ${proxy} \
--output ${name}-response-proxy.txt \
--stderr ${name}-verbose-proxy.txt
# Proxy access with no response header.
curl "${url}" \
--get --verbose \
--proxy ${proxy} \
--output ${name}-response-proxy-no-header.txt \
--stderr ${name}-verbose-proxy-no-header.txt
# Compare direct and proxy access.
echo '- compare with response header'
diff ${name}-response.txt ${name}-response-proxy.txt
echo '- compare with no response header'
diff ${name}-response-no-header.txt ${name}-response-proxy-no-header.txt
echo

# Test with a large bio page.
name='bio'
url='http://www.cs.cmu.edu/~prs/bio.html'
echo "GET ${url}"
# Direct access with response header.
curl "${url}" \
--get --verbose --include \
--output ${name}-response.txt \
--stderr ${name}-verbose.txt
# Direct access with no response header.
curl "${url}" \
--get --verbose \
--output ${name}-response-no-header.txt \
--stderr ${name}-verbose-no-header.txt
# Proxy access with response header.
curl "${url}" \
--get --verbose --include \
--proxy ${proxy} \
--output ${name}-response-proxy.txt \
--stderr ${name}-verbose-proxy.txt
# Proxy access with no response header.
curl "${url}" \
--get --verbose \
--proxy ${proxy} \
--output ${name}-response-proxy-no-header.txt \
--stderr ${name}-verbose-proxy-no-header.txt
# Compare direct and proxy access.
echo '- compare with response header'
diff ${name}-response.txt ${name}-response-proxy.txt
echo '- compare with no response header'
diff ${name}-response-no-header.txt ${name}-response-proxy-no-header.txt
echo

# Test with a large jpg.
name='jpg'
url='http://www.cs.cmu.edu/~dga/dga-headshot.jpg'
echo "GET ${url}"
# Direct access with response header.
curl "${url}" \
--get --verbose --include \
--output ${name}-response.txt \
--stderr ${name}-verbose.txt
# Direct access with no response header.
curl "${url}" \
--get --verbose \
--output ${name}-response-no-header.txt \
--stderr ${name}-verbose-no-header.txt
# Proxy access with response header.
curl "${url}" \
--get --verbose --include \
--proxy ${proxy} \
--output ${name}-response-proxy.txt \
--stderr ${name}-verbose-proxy.txt
# Proxy access with no response header.
curl "${url}" \
--get --verbose \
--proxy ${proxy} \
--output ${name}-response-proxy-no-header.txt \
--stderr ${name}-verbose-proxy-no-header.txt
# Compare direct and proxy access.
echo '- compare with response header'
diff ${name}-response.txt ${name}-response-proxy.txt
echo '- compare with no response header'
diff ${name}-response-no-header.txt ${name}-response-proxy-no-header.txt
echo

# Test CONNECT method.
name='connect'
url='https://www.cplusplus.com/reference/cstring/strcat/'
echo "CONNECT ${url}"
# Direct access with response header.
curl "${url}" \
--insecure --verbose --include \
--output ${name}-response.txt \
--stderr ${name}-verbose.txt
# Direct access with no response header.
curl "${url}" \
--insecure --verbose \
--output ${name}-response-no-header.txt \
--stderr ${name}-verbose-no-header.txt
# Proxy access with response header.
curl "${url}" \
--insecure --verbose --include \
--proxy ${proxy} \
--output ${name}-response-proxy.txt \
--stderr ${name}-verbose-proxy.txt
# Proxy access with no response header.
curl "${url}" \
--insecure --verbose \
--proxy ${proxy} \
--output ${name}-response-proxy-no-header.txt \
--stderr ${name}-verbose-proxy-no-header.txt
# Compare direct and proxy access.
echo '- compare with response header'
diff ${name}-response.txt ${name}-response-proxy.txt
echo '- compare with no response header'
diff ${name}-response-no-header.txt ${name}-response-proxy-no-header.txt
echo

# # Test with SCP wiki, which uses chunked transfer encoding.
# name='scp'
# url='http://scp-wiki-cn.wikidot.com/'
# echo "GET ${url}"
# # Direct access with response header.
# curl "${url}" \
# --get --verbose --include \
# --output ${name}-response.txt \
# --stderr ${name}-verbose.txt
# # Direct access with no response header.
# curl "${url}" \
# --get --verbose \
# --output ${name}-response-no-header.txt \
# --stderr ${name}-verbose-no-header.txt
# # Proxy access with response header.
# curl "${url}" \
# --get --verbose --include \
# --proxy ${proxy} \
# --output ${name}-response-proxy.txt \
# --stderr ${name}-verbose-proxy.txt
# # Proxy access with no response header.
# curl "${url}" \
# --get --verbose \
# --proxy ${proxy} \
# --output ${name}-response-proxy-no-header.txt \
# --stderr ${name}-verbose-proxy-no-header.txt
# # Compare direct and proxy access.
# echo '- compare with response header'
# diff ${name}-response.txt ${name}-response-proxy.txt
# echo '- compare with no response header'
# diff ${name}-response-no-header.txt ${name}-response-proxy-no-header.txt
# echo
