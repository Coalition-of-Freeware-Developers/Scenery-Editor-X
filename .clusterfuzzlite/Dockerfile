# ClusterFuzzLite build image: OSS-Fuzz's builder (Clang, libFuzzer, AFL++, the sanitizers) plus what source/Fuzz needs.
# Only the sources the fuzz targets compile and the sample projects the seed writer reads are copied in.
FROM gcr.io/oss-fuzz-base/base-builder:ubuntu-24-04
RUN apt-get update && apt-get install -y --no-install-recommends cmake ninja-build zlib1g-dev && rm -rf /var/lib/apt/lists/*
COPY source $SRC/scenery-editor-x/source
COPY dependency/json $SRC/scenery-editor-x/dependency/json
COPY docs/MySceneryProject.edx $SRC/scenery-editor-x/docs/MySceneryProject.edx
COPY docs/XCoder/earth.wed.xml $SRC/scenery-editor-x/docs/XCoder/earth.wed.xml
WORKDIR $SRC/scenery-editor-x
COPY .clusterfuzzlite/build.sh $SRC/
