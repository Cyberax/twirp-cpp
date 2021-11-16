from conans import ConanFile, CMake, tools
import re, os

def get_version():
    git = tools.Git()
    try:
        branch = git.get_branch()
        # Check if the current revision is a release branch and use it directly
        if re.match("v[0-9]+\\.[0-9]+.*", branch):
            return "%s" % branch
        # If not, just use the commit hash
        return "%s_%s" % (git.get_branch(), git.get_revision()[0:8])
    except:
        return None

class TwirpTestProject(ConanFile):
    requires = [
        "twirp-cpp/%s" % get_version(),
        "cpp-httplib/0.9.7",
        "jsoncpp/1.9.3",
        "openssl/1.1.1l",
        "gtest/1.10.0",
        "protobuf/3.12.4",
        "abseil/20210324.2",
    ]
    generators = "cmake"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        os.chdir("bin")
        self.run("./gproto")
