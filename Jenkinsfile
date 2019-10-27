
def compilers = [ 'gcc49', 'gcc5', 'gcc6', 'gcc7', 'gcc8' ]
def container_init = {
    sh "sudo apt-get update"
    sh "sudo apt-get install -y netbase"
}

conan_build(
    /**/ compilers:   compilers,
    /**/ environment: [ CONAN_BUILD_LIBCXX: "libstdc++11" ],
    /**/ stages:      compilers.collectEntries { [ ( "init." + it ): container_init ] },
)
