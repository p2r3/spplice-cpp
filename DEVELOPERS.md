# Building manually

Running `./autobuild.sh --both` on a Debian (Bookworm) environment should install all needed dependencies, as well as build the project.

The `--both` signals it to build for both Linux, and Windows. Omitting it will build for linux, and specifying `--win` will build just a windows package.

# Building with Docker

Build the builder:
```sh
docker build -t spplice3-builder -f builder.Dockerfile .
```

Build the project
```sh
docker run --rm -v $(pwd)/dist:/dist -v $(pwd):/src spplice3-builder
```

The first command will take a while to run (it's going to compile Qt5 for Linux & Windows).

Note that you only need to run the second docker command (`docker run ...`) to (re-)build the project, unless a dependency changed.
