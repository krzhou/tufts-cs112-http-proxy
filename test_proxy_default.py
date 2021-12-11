import os
import shutil
import subprocess
import tempfile
import time
import unittest


class TestProxyDefault(unittest.TestCase):
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
        port = 9160
        cls.proxy_process = subprocess.Popen(["./proxy", str(port)], cwd=cls.test_root)
        cls.proxy_url = "localhost:" + str(port)
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


    def test_get(self):
        # Medium sized website.
        url ="http://www.cs.cmu.edu/~prs/bio.html"
        name = "get-bio"
        self.compare_curl_result(url, name)

        # Website with a large picture.
        url="http://www.cs.cmu.edu/~dga/dga-headshot.jpg"
        name = "get-headshot"
        self.compare_curl_result(url, name)


    def test_get_chunked(self):
        ''' Test chunked transfer encoding. '''
        url ="http://scp-wiki-cn.wikidot.com/"
        name = "chunked-scp"
        self.compare_curl_result(url, name)


    def test_connect(self):
        url ="https://www.eecs.tufts.edu"
        name = "connect-eecs"
        self.compare_curl_result(url, name)

        self.compare_curl_result(url, name)
        url ="https://www.tufts.edu"
        name = "connect-tufts"
        self.compare_curl_result(url, name)

        url ="https://www.cplusplus.com"
        name = "connect-cplusplus"
        self.compare_curl_result(url, name)
