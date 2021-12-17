###############################################################
#
#               bench_proxy_ssl_interception.py
#
#     Final Project: High Performance HTTP Proxy
#     Author:  Keren Zhou (kzhou), Ruiyuan Gu (rgu03)
#     Date: 2021-12-11
#
#     Summary:
#     Performance tests for proxy running in SSL interception
#     mode.
#
#     Usage: python3 bench_proxy_default.py [port]
#     where [port] is the port that the proxy listens on,
#     9999 by default.
#
###############################################################

import os
import shutil
import subprocess
import sys
import tempfile
import time
import unittest


class TestProxyDefault(unittest.TestCase):
    CSV_format = False  # Whether to print in CSV format.
    PORT = 9999  # Port that the proxy listens on.
    TIMEOUT = 10  # Timeout for website access.
    URLS = [
        "www.google.com",
        "www.youtube.com",
        "www.facebook.com",
        "www.baidu.com",
        "www.wikipedia.org",
        "www.qq.com",
        "www.taobao.com",
        "www.yahoo.com",
        "www.tmall.com",
        "www.amazon.com",
        "www.twitter.com",
        "www.sohu.com",
        "www.live.com",
        "www.jd.com",
        "www.vk.com",
        "www.instagram.com",
        "www.sina.com.cn",
        "www.weibo.com",
        "www.reddit.com",
        "login.tmall.com",
        "www.360.cn",
        "www.yandex.ru",
        "www.linkedin.com",
        "www.blogspot.com",
        "www.netflix.com",
        "www.twitch.tv",
        "www.whatsapp.com",
        "www.pornhub.com",
        "www.yahoo.co.jp",
        "www.csdn.net",
        "www.alipay.com",
        "www.microsoftonline.com",  # Unreachable
        "www.naver.com",
        "pages.tmall.com",
        "www.microsoft.com",
        "www.livejasmin.com",
        "www.aliexpress.com",
        "www.bing.com",
        "www.ebay.com",
        "www.github.com",
        "www.tribunnews.com",
        "www.google.com.hk",
        "www.amazon.co.jp",
        "www.stackoverflow.com",
        "www.mail.ru",
        "www.okezone.com",
        "www.google.co.in",
        "www.office.com",
        "www.xvideos.com",
        "www.msn.com",
        "www.paypal.com",
        "www.bilibili.com",
        "www.hao123.com",
        "www.imdb.com",
        "www.t.co",
        "www.fandom.com",
        "www.imgur.com",
        "www.xhamster.com",
        "www.163.com",
        "www.wordpress.com",
        "www.apple.com",
        "www.soso.com",
        "www.google.com.br",
        "www.booking.com",
        "www.xinhuanet.com",
        "www.adobe.com",
        "www.pinterest.com",
        "www.amazon.de",
        "www.amazon.in",
        "www.dropbox.com",
        "www.bongacams.com",
        "www.google.co.jp",
        "www.babytree.com",
        "detail.tmall.com",
        "www.tumblr.com",
        "www.google.ru",
        "www.google.fr",
        "www.google.de",
        "www.so.com",
        "www.cnblogs.com",
        "www.quora.com",
        "www.amazon.co.uk",
        "www.detik.com",
        "www.google.cn",
        "www.bbc.com",
        "www.force.com",
        "www.deloplen.com",  # Unreachable.
        "www.salesforce.com",
        "www.pixnet.net",
        "www.ettoday.net",
        "www.cnn.com",
        "www.onlinesbi.com",
        "www.roblox.com",
        "www.aparat.com",
        "www.thestartmagazine.com",
        "www.bbc.co.uk",
        "www.google.es",
        "www.amazonaws.com",
        "www.google.it",
        "www.tianya.cn",
    ]  # URLs for benchmark.


    @classmethod
    def setUpClass(cls):
        print("==== proxy bench mark for SSL interception mode ====")

        # Setup a temp dir as the test dir.
        repo_root = os.path.join(os.path.dirname(__file__))
        print("repo_root:", repo_root)
        proxy_path = os.path.join(repo_root, "proxy")
        if not os.path.exists(proxy_path):
            raise Exception("proxy not found; please build the project first")
        cert_path = os.path.join(repo_root, "cert.pem")
        if not os.path.exists(cert_path):
            raise Exception("cert.pem not found")
        key_path = os.path.join(repo_root, "key.pem")
        if not os.path.exists(key_path):
            raise Exception("key.pem not found")
        cls.test_root = tempfile.mkdtemp()
        print("test_root:", cls.test_root)
        shutil.copy(proxy_path, cls.test_root)
        shutil.copy(cert_path, cls.test_root)
        shutil.copy(key_path, cls.test_root)

        # Start proxy under the test dir.
        # cls.proxy_out = open("proxy_out.txt", "w")
        cls.proxy_out = open(os.path.join(cls.test_root, "proxy_out.txt"), "w")
        cls.proxy_process = subprocess.Popen(["./proxy", str(cls.PORT),
                                              "cert.pem", "key.pem"],
                                             stdout=subprocess.PIPE,
                                             stderr=cls.proxy_out,
                                             cwd=cls.test_root)
        cls.proxy_url = "localhost:" + str(cls.PORT)
        time.sleep(1)  # Wait for proxy to start.


    @classmethod
    def tearDownClass(cls):
        # Shut down the proxy.
        cls.proxy_process.kill()
        cls.proxy_process.wait()
        cls.proxy_out.close()

        # Cleanup the test dir.
        shutil.rmtree(cls.test_root)

        print("==== benchmark end ====")


    def get_page_size(self, url):
        '''
        @brief Get page size using curl.
        @param url URL of the website to access.
        '''
        # E.g. curl -so /dev/null "www.google.com" -w '%{size_download}\n'
        stdout_path = os.path.join(self.test_root, "pagesize.txt")
        stdout_file = open(stdout_path, "w")
        subprocess.run(["curl", url, "-so", "/dev/null", "-w", "%{size_download}\n"],
                       stdout=stdout_file,
                       cwd=self.test_root)
        stdout_file.close()
        stdout_file = open(stdout_path, "r")
        line = stdout_file.readline()
        stdout_file.close()
        return int(line)


    def compare_curl_time(self, url):
        '''
        @brief Compare curl time of accessing a website directly or via proxy.
        @param url URL of the website to access.
        '''
        if not self.CSV_format:
            print(url)

        # Proxy without cache.
        try:
            stdout_file = "proxy-uncached-stdout.txt"
            stderr_file = "proxy-uncached-stderr.txt"
            proxy_uncached_start = time.time()
            subprocess.run(["curl", url, "--get", "--verbose", "--insecure",
                "--proxy", self.proxy_url,
                "--output", stdout_file,
                "--stderr", stderr_file],
                timeout = self.TIMEOUT,
                cwd=self.test_root)
            proxy_uncached_elapsed = time.time() - proxy_uncached_start
        except subprocess.TimeoutExpired:
            print("fail to access {}".format(url))
            return -1, -1, -1
            print()

        # Cached proxy.
        try:
            stdout_file = "proxy-cached-stdout.txt"
            stderr_file = "proxy-cached-stderr.txt"
            proxy_cached_start = time.time()
            subprocess.run(["curl", url, "--get", "--verbose", "--insecure",
                "--proxy", self.proxy_url,
                "--output", stdout_file,
                "--stderr", stderr_file],
                timeout = self.TIMEOUT,
                cwd=self.test_root)
            proxy_cached_elapsed = time.time() - proxy_cached_start
        except subprocess.TimeoutExpired:
            print("fail to access {}".format(url))
            return -1, -1, -1
            print()

        # Direct curl access.
        try:
            stdout_file = "direct-stdout.txt"
            stderr_file = "direct-stderr.txt"
            direct_start = time.time()
            subprocess.run(["curl", url, "--get", "--verbose",
                "--output", stdout_file,
                "--stderr", stderr_file],
                timeout = self.TIMEOUT,
                cwd=self.test_root)
            direct_elapsed = time.time() - direct_start
        except subprocess.TimeoutExpired:
            print("fail to access {}".format(url))
            return -1, -1, -1
            print()
        
        if self.CSV_format:
            # CSV format.
            print("{}, {}, {}, {}".format(format(proxy_uncached_elapsed, ".3f"),
                                        format(proxy_cached_elapsed, ".3f"), 
                                        format(direct_elapsed, ".3f"),
                                        self.get_page_size(url)))
        else:
            # Compare elapsed time.
            print("uncached proxy elapsed: {} s".format(format(proxy_uncached_elapsed, ".3f")))
            print("cached   proxy elapsed: {} s".format(format(proxy_cached_elapsed, ".3f")))
            print("direct         elapsed: {} s".format(format(direct_elapsed, ".3f")))
            print("uncached proxy gain   : {} s".format(format(proxy_uncached_elapsed - direct_elapsed, ".3f")))
            print("uncached proxy gain%  : {} s".format(format((proxy_uncached_elapsed - direct_elapsed) / direct_elapsed, ".3%")))
            print("cached   proxy gain   : {} s".format(format(proxy_cached_elapsed - direct_elapsed, ".3f")))
            print("cached   proxy gain%  : {} s".format(format((proxy_cached_elapsed - direct_elapsed) / direct_elapsed, ".3%")))
            print("page size             : {} Bytes".format(self.get_page_size(url)))
            print()

        return proxy_uncached_elapsed, proxy_cached_elapsed, direct_elapsed


    def test_performance(self):
        unreachable = 0  # #websites fail to access.
        total_uncached = 0.0  # Total elapsed time of accessing by uncached proxy.
        total_cached = 0.0  # Total elapsed time of accessing by cached proxy.
        total_direct = 0.0  # Total elapsed time of direct accessing.
        for url in self.URLS:
            uncached, cached, direct = self.compare_curl_time(url)
            if uncached < 0 or cached < 0 or direct < 0:
                unreachable += 1
                continue
            total_uncached += uncached
            total_cached += cached
            total_direct += direct

        if not self.CSV_format:
            # Report elapsed time for uncached proxy access, cached proxy access
            # and direct access.
            print("==== Summary ====")
            print("uncached proxy elapsed: {} s".format(format(total_uncached, ".3f")))
            print("cached   proxy elapsed: {} s".format(format(total_cached, ".3f")))
            print("direct         elapsed: {} s".format(format(total_direct, ".3f")))
            print("uncached proxy gain   : {} s".format(format(total_uncached - total_direct, ".3f")))
            print("uncached proxy gain%  : {} s".format(format((total_uncached - total_direct) / total_direct, ".3%")))
            print("cached   proxy gain   : {} s".format(format(total_cached - total_direct, ".3f")))
            print("cached   proxy gain%  : {} s".format(format((total_cached - total_direct) / total_direct, ".3%")))
            print("unreachable websites  : {}".format(unreachable))
            print()


if __name__ == "__main__":
    # Parse command line arguments.
    if (len(sys.argv) == 2):
        TestProxyDefault.PORT = int(sys.argv.pop())

    unittest.main()
