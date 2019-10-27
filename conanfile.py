# BSD 2-Clause License
#
# Copyright (c) 2020, Thomas Santanello
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from conans import ConanFile, tools, CMake

class CppuriConan(ConanFile):
    name = "cppuri"
    license = "BSD-2-Clause"
    author = "Thomas Santanello <tcsantanello@gmail.com>"
    description = "URI parser for C++"
    url = "https://gitea.apps.nemesus.homeip.net/c-cpp-projects/cppuri.git"
    topics = ("uri")
    exports_sources = ""
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def export_sources(self):
        git = tools.Git(".")
        for file in git.run("ls-tree -r HEAD --name-only").split("\n"):
            self.copy(file)

    def build(self):
        vers = self.version.replace("-dirty", "")
        cmake = CMake(self)
        cmake.verbose = True
        print("Using Version: {}".format(vers))
        cmake.definitions["VERSION"] = vers
        cmake.configure()
        cmake.build()

    def package(self):
        for suffix in ["h", "hh", "hpp", "H", "hxx", "hcc"]:
            self.copy("*.{}".format(suffix))
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so*", dst="lib", keep_path=False, symlinks=True)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
