###############################################################
#
#                   test_proxy_default.py
#
#     Final Project: High Performance HTTP Proxy
#     Author:  Keren Zhou (kzhou), Ruiyuan Gu (rgu03)
#     Date: 2021-12-10
#
#     Summary:
#     Intergration tests for proxy running in default mode.
#
#     Usage: python3 test_proxy_default.py [port]
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
        # Setup a temp dir as the test dir.
        repo_root = os.path.join(os.path.dirname(__file__))
        print("repo_root:", repo_root)
        proxy_path = os.path.join(repo_root, "proxy")
        if not os.path.exists(proxy_path):
            raise Exception("proxy not found; please build the project first")
        cls.test_root = tempfile.mkdtemp()
        print("test_root:", cls.test_root)
        shutil.copy(proxy_path, cls.test_root)

        # Start proxy under the test dir.
        cls.proxy_process = subprocess.Popen(["./proxy", str(cls.PORT)],
                                             cwd=cls.test_root)
        cls.proxy_url = "localhost:" + str(cls.PORT)
        time.sleep(1)  # Wait for proxy to start.


    @classmethod
    def tearDownClass(cls):
        # Shut down the proxy.
        cls.proxy_process.kill()
        cls.proxy_process.wait()

        # Cleanup the test dir.
        shutil.rmtree(cls.test_root)


    def compare_curl_result(self, url, name):
        '''
        @brief Compare curl result of accessing a website directly or via proxy.
        @param url URL of the website to access.
        @param name Head of output filename.
        '''
        print("TEST {}".format(url))

        # Direct curl access.
        direct_stdout = name + "-direct-stdout.txt"
        direct_stderr = name + "-direct-stderr.txt"
        subprocess.run(["curl", url, "--get", "--verbose",
            "--output", direct_stdout,
            "--stderr", direct_stderr],
            cwd=self.test_root)

        # Curl access via proxy.
        proxy_stdout = name + "-proxy-stdout.txt"
        proxy_stderr = name + "-proxy-stderr.txt"
        subprocess.run(["curl", url, "--get", "--verbose",
            "--proxy", self.proxy_url,
            "--output", proxy_stdout,
            "--stderr", proxy_stderr],
            cwd=self.test_root)

        # Compare response without header.
        result = subprocess.run(["diff", direct_stdout, proxy_stdout],
            cwd=self.test_root)
        self.assertEqual(result.returncode, 0)

        print("PASS")


    def test_get_01(self):
        # Medium sized website.
        self.compare_curl_result(
            url="http://www.cs.cmu.edu/~prs/bio.html",
            name="get-bio",
        )


    def test_get_02(self):
        # Website with a large picture.
        self.compare_curl_result(
            url="http://www.cs.cmu.edu/~dga/dga-headshot.jpg",
            name="get-headshot",
        )


    @unittest.skip("broken")  # Response may vary each time.
    def test_get_chunked_01(self):
        ''' Test chunked transfer encoding. '''
        self.compare_curl_result(
            url="http://scp-wiki-cn.wikidot.com/",
            name="chunked-scp",
        )

    @unittest.skip("broken")  # Need Tufts VPN to access.
    def test_connect_01(self):
        self.compare_curl_result(
            url="https://www.eecs.tufts.edu",
            name="connect-eecs",
        )


    def test_connect_02(self):
        self.compare_curl_result(
            url="https://www.tufts.edu",
            name="connect-tufts",
        )


    def test_connect_03(self):
        self.compare_curl_result(
            url="https://www.cplusplus.com",
            name="connect-cplusplus",
        )


    def test_unreachable(self):
        ''' Test with an unreachable website. '''
        name = "unreachable"
        url = "http://www.1234567890.com"
        proxy_stdout = name + "-proxy-stdout.txt"
        proxy_stderr = name + "-proxy-stderr.txt"
        try:
            subprocess.run(["curl", url, "--get", "--verbose",
                "--proxy", self.proxy_url,
                "--output", proxy_stdout,
                "--stderr", proxy_stderr],
                timeout=3,
                cwd=self.test_root)
        except subprocess.TimeoutExpired:
            return
        assertTrue(False)


if __name__ == "__main__":
    # Parse command line arguments.
    if (len(sys.argv) == 2):
        TestProxyDefault.PORT = int(sys.argv.pop())

    unittest.main()
