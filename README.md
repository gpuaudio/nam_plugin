# nam_plugin

## Build instructions

1. Install dependencies. You might have to generate an Identity Token on Artifactory and set it as an environment variable to `GPUAUDIO_ART_KEY` in order for CMake to properly fetch Sofilib, which is one of the dependencies.

```
conan install . --update --install-folder @BUILD-MD -pr:h Windows-Debug-x86_64-VS2022-MDd-v143-14.34.31933-std20-10.0.22621.0-cuda12.0-hip5.5.1 -pr:b Windows-Debug-x86_64-VS2022-MDd-v143-14.34.31933-std20-10.0.22621.0-cuda12.0-hip5.5.1

conan install . --update --install-folder @BUILD-MD -pr:h Windows-RelWithDebInfo-x86_64-VS2022-MD-v143-14.34.31933-std20-10.0.22621.0-cuda12.0-hip5.5.1 -pr:b Windows-RelWithDebInfo-x86_64-VS2022-MD-v143-14.34.31933-std20-10.0.22621.0-cuda12.0-hip5.5.1

conan install . --update --install-folder @BUILD-MD -pr:h Windows-Release-x86_64-VS2022-MD-v143-14.34.31933-std20-10.0.22621.0-cuda12.0-hip5.5.1 -pr:b Windows-Release-x86_64-VS2022-MD-v143-14.34.31933-std20-10.0.22621.0-cuda12.0-hip5.5.1
```

2. Generate build files with CMake

```
cmake --preset MD
```

3. Build. Replace -j16 with your number of cores.

```
cmake --build --preset Debug-MD -j16

cmake --build --preset RelWithDebInfo-MD -j16

cmake --build --preset Release-MD -j16
```
