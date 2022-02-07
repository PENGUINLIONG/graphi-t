# GraphiT

Handy tools & graphics API abstraction for blazing fast prototyping.

There exist lots of abstractions of graphics APIs (like [wgpu](https://github.com/gfx-rs/wgpu) and Unreal Engine RHI) but most of them intentionally hide their implementations and data structures over encapsulation and inheritance. Such design is extremely confusing and exhausting in my research projects. [GraphiT](https://github.com/PENGUINLIONG/graphi-t) is thus one of my attempts to get things done more easily.

GraphiT has all (or most of those) you need for fast prototyping:

- [Argument parsing](https://github.com/PENGUINLIONG/graphi-t/blob/main/include/gft/args.hpp) with flexibility;
- [JSON ser/de](https://github.com/PENGUINLIONG/graphi-t/blob/main/include/gft/json.hpp) for configuration and structured serialization;
- [Logging](https://github.com/PENGUINLIONG/graphi-t/blob/main/include/gft/json.hpp) with severity filtering & customized output callback;
- [Hardware Abstraction Layer](https://github.com/PENGUINLIONG/graphi-t/blob/main/include/gft/hal/hal.hpp) over [Vulkan](https://github.com/PENGUINLIONG/graphi-t/blob/main/include/gft/vk.hpp) API with every handle accessible;
- [Glslang](https://github.com/PENGUINLIONG/graphi-t/blob/main/include/gft/glslang.hpp) integration for on-the-flight shader compilation;
- [Descriptive statistics](https://github.com/PENGUINLIONG/graphi-t/blob/main/include/gft/stats.hpp) to collect minimum, maximum, mean, etc.;
- [BMP image writer, text editing, timer](https://github.com/PENGUINLIONG/graphi-t/blob/main/include/gft/util.hpp) and a lot more utility functions.

## Run Examples

You can use the following commands to run the GraphiT demo on Windows host and Android device attached via adb:
```powershell
./Run-Host.ps1 -a Demo # Run on host.

./Run-Android.ps1 -a Demo # Run on attached Android device over adb.
```

## Use GraphiT in Your Project

You can create a new repository on Github by clicking at the `Use this template` button in [GraphiT-Template](https://github.com/PENGUINLIONG/GraphiT-Template). It comes with a 'hello world' program using GraphiT as a Git submodule.

GraphiT can also be integrated by `add_subdirectory` like many other CMake projects.

## License

This project is licensed under either of

* Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or http://www.apache.org/licenses/LICENSE-2.0)
* MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.

## Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in GraphiT by you, as defined in the Apache-2.0 license, shall be dual licensed as above, without any additional terms or conditions.
