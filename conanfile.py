from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class rsltestRecipe(ConanFile):
    name = "rsl-log"
    version = "0.1"
    package_type = "library"

    # Optional metadata
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of mypkg package here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False], "fPIC": [True, False],
        "tests": [True, False],
        "coverage": [True, False],
        "examples": [True, False],
        "editable": [True, False]
    }

    default_options = {"shared": False, "fPIC": True, "tests": False, "coverage": False, "examples": False, "editable": False}
    exports_sources = "CMakeLists.txt", "src/*", "include/*", "example/*", "test/*"

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def requirements(self):
        self.requires("rsl-util/0.1", transitive_headers=True, transitive_libs=True)
        self.test_requires("rsl-test/0.1")

    def layout(self):
        cmake_layout(self)
    
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={
                    "ENABLE_COVERAGE": self.options.coverage,
                    "BUILD_EXAMPLES": self.options.examples,
                    "BUILD_TESTING": self.options.tests
                })
        cmake.build()
        if self.options.editable:
            # package is in editable mode - make sure it's installed after building
            cmake.install()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "rsl")
        self.cpp_info.components["log"].set_property("cmake_target_name", "rsl::log")
        self.cpp_info.components["log"].includedirs = ["include"]
        self.cpp_info.components["log"].libdirs = ["lib"]
        self.cpp_info.components["log"].requires = []
        self.cpp_info.components["log"].libs = ["rsl-log"]