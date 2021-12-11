###############################################################
#
#                  bench_proxy_default.py
#
#     Final Project: High Performance HTTP Proxy
#     Author:  Keren Zhou (kzhou), Ruiyuan Gu (rgu03)
#     Date: 2021-12-11
#
#     Summary:
#     Performance tests for proxy running in default mode.
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
    PORT = 9999  # Port that the proxy listens on.

    @classmethod
    def setUpClass(cls):
        print("==== proxy bench mark for default mode ====")

        # Setup a temp dir as the test dir.
        repo_root = os.path.join(os.path.dirname(__file__))
        print("repo_root:", repo_root)
        proxy_path = os.path.join(repo_root, "proxy")
        if not os.path.exists(proxy_path):
            raise Exception("proxy not found; please build the project first")
        cls.test_root = tempfile.mkdtemp()
        print("test_root:", cls.test_root)
        shutil.copy(proxy_path, cls.test_root)

        # Start proxy under the test dir and redirect its output.
        # cls.proxy_out = open("proxy_out.txt", "w")
        cls.proxy_out = open(os.path.join(cls.test_root, "proxy_out.txt"), "w")
        cls.proxy_process = subprocess.Popen(["./proxy", str(cls.PORT)],
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


    def compare_curl_time(self, url, name):
        '''
        @brief Compare curl time of accessing a website directly or via proxy.
        @param url URL of the website to access.
        @param name Head of output filename.
        '''
        print(url)

        # Proxy without cache.
        proxy_stdout = name + "-proxy-stdout.txt"
        proxy_stderr = name + "-proxy-stderr.txt"
        proxy_uncached_start = time.time()
        subprocess.run(["curl", url, "--get", "--verbose",
            "--proxy", self.proxy_url,
            "--output", proxy_stdout,
            "--stderr", proxy_stderr],
            cwd=self.test_root)
        proxy_uncached_elapsed = time.time() - proxy_uncached_start

        # Cached proxy.
        proxy_stdout = name + "-proxy-stdout.txt"
        proxy_stderr = name + "-proxy-stderr.txt"
        proxy_cached_start = time.time()
        subprocess.run(["curl", url, "--get", "--verbose",
            "--proxy", self.proxy_url,
            "--output", proxy_stdout,
            "--stderr", proxy_stderr],
            cwd=self.test_root)
        proxy_cached_elapsed = time.time() - proxy_cached_start

        # Direct curl access.
        direct_stdout = name + "-direct-stdout.txt"
        direct_stderr = name + "-direct-stderr.txt"
        direct_start = time.time()
        subprocess.run(["curl", url, "--get", "--verbose",
            "--output", direct_stdout,
            "--stderr", direct_stderr],
            cwd=self.test_root)
        direct_elapsed = time.time() - direct_start

        # Compare elapsed time.
        print("== uncached ==")
        print("proxy  elapsed: {} s".format(format(proxy_uncached_elapsed, ".3f")))
        print("direct elapsed: {} s".format(format(direct_elapsed, ".3f")))
        print("diff          : {} s".format(format(proxy_uncached_elapsed - direct_elapsed, ".3f")))
        print("diff%         : {} s".format(format((proxy_uncached_elapsed - direct_elapsed) / direct_elapsed, ".3%")))
        print("== cached ==")
        print("proxy  elapsed: {} s".format(format(proxy_cached_elapsed, ".3f")))
        print("direct elapsed: {} s".format(format(direct_elapsed, ".3f")))
        print("diff          : {} s".format(format(proxy_cached_elapsed - direct_elapsed, ".3f")))
        print("diff%         : {} s".format(format((proxy_cached_elapsed - direct_elapsed) / direct_elapsed, ".3%")))
        print()

        return proxy_uncached_elapsed, proxy_cached_elapsed, direct_elapsed


    def test_performance(self):
        urls = [
            "http://www.cs.cmu.edu/~prs/bio.html",
            "http://www.cs.cmu.edu/~dga/dga-headshot.jpg",
            "http://scp-wiki-cn.wikidot.com/",
            "https://www.eecs.tufts.edu",
            "https://www.tufts.edu",
            "https://www.cplusplus.com",
        ]
        total_uncached = 0.0
        total_cached = 0.0
        total_direct = 0.0
        for i in range(len(urls)):
            uncached, cached, direct = self.compare_curl_time(urls[i], "bench")
            total_uncached += uncached
            total_cached += cached
            total_direct += direct

        # Report elapsed time for uncached proxy access, cached proxy access and
        # direct access.
        print("Summary:")
        print("== uncached ==")
        print("proxy  total: {} s".format(format(total_uncached, ".3f")))
        print("direct total: {} s".format(format(total_direct, ".3f")))
        print("diff        : {} s".format(format(total_uncached - total_direct, ".3f")))
        print("diff%       : {} s".format(format((total_uncached - total_direct) / total_direct, ".3%")))
        print("== cached ==")
        print("proxy  total: {} s".format(format(total_cached, ".3f")))
        print("direct total: {} s".format(format(total_direct, ".3f")))
        print("diff        : {} s".format(format(total_cached - total_direct, ".3f")))
        print("diff%       : {} s".format(format((total_cached - total_direct) / total_direct, ".3%")))
        print()


if __name__ == "__main__":
    # Parse command line arguments.
    if (len(sys.argv) == 2):
        TestProxyDefault.PORT = int(sys.argv.pop())

    unittest.main()
