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
import signal
import shutil
import subprocess
import sys
import tempfile
import time
import unittest


class TestProxyDefault(unittest.TestCase):
    CSV_format = False  # Whether to print in CSV format.
    PORT = 9999  # Port that the proxy listens on.
    TIMEOUT = 20  # Timeout for website access.
    URLS = [
        # HTTP websites.
        "http://www.cs.cmu.edu/~prs/bio.html",
        "http://www.cs.cmu.edu/~dga/dga-headshot.jpg",
        "http://info.cern.ch",
        "http://brightbeautifulshiningsunrise.neverssl.com/online",
        "http://www.testingmcafeesites.com/",
        "http://www.softwareqatest.com",
        "http://www.http2demo.io/",
        "http://scp-wiki-cn.wikidot.com/",
        "http://scp-jp.wikidot.com/",

        # Top 50 websites.
       "https://www.google.com/",
       "https://www.youtube.com/",
       "https://www.facebook.com/",
       "https://twitter.com/",
       "https://www.instagram.com/",
       "https://www.baidu.com/",
       "https://www.wikipedia.org/",
       "https://yandex.ru/",
       "https://www.yahoo.com/",
       "https://www.xvideos.com/",
       "https://www.whatsapp.com/",
       "https://www.amazon.com/",
       "https://www.xnxx.com/",
       "https://www.netflix.com/",
       "https://outlook.live.com/owa/",
       "https://www.yahoo.co.jp/",
       "https://www.pornhub.com/",
       "https://zoom.us/",
       "https://www.office.com/",
       "https://www.reddit.com/",
       "https://vk.com/",
       "https://www.tiktok.com/",
       "https://xhamster.com/",
       "https://www.linkedin.com/",
       "https://discord.com/",
       "https://www.naver.com/",
       "https://www.twitch.tv/",
       "https://www.bing.com/",
       "https://www.microsoft.com/en-us/",
       "https://mail.ru/",
       "https://www.roblox.com/",
       "https://duckduckgo.com/",
       "https://www.pinterest.com/",
       "https://www.samsung.com/us/",
       "https://www.qq.com/",
       "https://www.msn.com/",
       "https://news.yahoo.co.jp/",
       "https://www.bilibili.com/",
       "https://www.ebay.com/",
       "https://www.google.com.br/",
       "https://www.globo.com/",
       "https://www.fandom.com/",
       "https://ok.ru/",
       "https://docomo.ne.jp/",
       "UNREACHABLE",  # realsrv.com,
       "https://www.bbc.com/",
       "https://www.accuweather.com/",
       "https://www.amazon.co.jp/",
       "https://www.walmart.com/",
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


    @classmethod
    def tearDownClass(cls):
        # Cleanup the test dir.
        shutil.rmtree(cls.test_root)

        print("==== benchmark end ====")


    def start_proxy(cls):
        # Start proxy under the test dir.
        cls.proxy_process = subprocess.Popen(["./proxy", str(cls.PORT),
                                               "cert.pem", "key.pem"],
                                              stdout=subprocess.DEVNULL,
                                              stderr=subprocess.DEVNULL,
                                              cwd=cls.test_root)
        cls.proxy_url = "localhost:" + str(cls.PORT)
        time.sleep(1)  # Wait for proxy to start.


    def stop_proxy(cls):
        # Shut down the proxy.
        cls.proxy_process.send_signal(signal.SIGINT)
        cls.proxy_process.wait()


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
            print()
            return -1, -1, -1

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
            print()
            return -1, -1, -1

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
            print()
            return -1, -1, -1
        
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
            self.start_proxy()

            uncached, cached, direct = self.compare_curl_time(url)
            if uncached < 0 or cached < 0 or direct < 0:
                unreachable += 1
            else:
                total_uncached += uncached
                total_cached += cached
                total_direct += direct

            self.stop_proxy()

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
