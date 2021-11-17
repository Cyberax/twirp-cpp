import os

from conans import ConanFile, tools
import re


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


class InjectConan(ConanFile):
    name = "twirp-cpp"
    settings = "os", "arch"
    version = get_version()

    def export_sources(self):
        self.copy("src/*")
        self.copy("include/*")
        with tools.chdir(os.path.join(self.export_sources_folder, "src")):
            self.run('go mod vendor')

    def build(self):
        os_name = str(self.settings.os).lower()
        arch = str(self.settings.arch)
        if os_name in ['macos', "ios", "watchos", "tvos"]:
            os_name = 'darwin'
        if arch == 'x86_64':
            arch = 'amd64'
        if arch == 'armv8' or arch == 'arm':
            arch = 'arm64'
        print("Building for OS: %s/%s" % (os_name, arch))        

        suffix = ""
        if os_name == 'windows':
            suffix = ".exe"
        with tools.chdir(os.path.join(self.source_folder, "src")):
            self.run("GOFLAGS=\"-mod=vendor\" GOOS=%s GOARCH=%s "
                     "go build -o ../protoc-gen-twirpcpp%s twirpcpp/main/main.go" % (os_name, arch, suffix), )
            self.run("GOFLAGS=\"-mod=vendor\" GOOS=%s GOARCH=%s "
                     "go build -o ../protodump%s dumper/protodump.go" % (os_name, arch, suffix), )

    def package(self):
        self.copy("protoc-gen-twirpcpp", dst='bin', keep_path=False)
        self.copy("protodump", dst='bin', keep_path=False)
        # Windows needs a suffix
        self.copy("protoc-gen-twirpcpp.exe", dst='bin', keep_path=False)
        self.copy("protodump.exe", dst='bin', keep_path=False)

        self.copy("include/*", dst='.', keep_path=True)

    def deploy(self):
        self.copy("*", src="bin", dst="bin")

    def package_info(self):
        bindir = os.path.join(self.package_folder, "bin")
        self.output.info("Appending PATH environment variable: {}".format(bindir))
        self.env_info.PATH.append(bindir)
