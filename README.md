<div align="center">

# sandbox

`sandbox` is a C++ executable.

![top-language-badge](https://img.shields.io/github/languages/top/ensketch/sandbox.svg?style=for-the-badge)
![code-size-badge](https://img.shields.io/github/languages/code-size/ensketch/sandbox.svg?style=for-the-badge)
![repo-size-badge](https://img.shields.io/github/repo-size/ensketch/sandbox.svg?style=for-the-badge)
[![license-badge](https://img.shields.io/github/license/ensketch/sandbox.svg?style=for-the-badge&color=blue)](#copyright-and-license)

![GitHub commit activity](https://img.shields.io/github/commit-activity/y/ensketch/sandbox?style=for-the-badge)
![GitHub last commit (by committer)](https://img.shields.io/github/last-commit/ensketch/sandbox?style=for-the-badge)
![GitHub tag (with filter)](https://img.shields.io/github/v/tag/ensketch/sandbox?style=for-the-badge)

[![cppget.org](https://img.shields.io/website/https/cppget.org/ensketch-sandbox.svg?down_message=offline&label=cppget.org&style=for-the-badge&up_color=blue&up_message=online)](https://cppget.org/ensketch-sandbox)

</div>

## Requirements and Dependencies

`sandbox` uses the [standard `build2` CI service][ci.cppget.org] to build and test its source code with a [variety of build configurations][ci.cppget.org-build-configs].
For a thorough list of requirements, see its [`manifest`][manifest] file.
To get detailed information on the current build status, see [Development Setup](#development-setup).
<!-- To get the build status of already published package versions, see [the package's build status report]. -->
Below is a concise list of requirements and dependencies to build and use this project.

**Architecture:**
- x86_64 | aarch64

**Operating System:**
- Linux | MacOS | Windows | FreeBSD

**C++ Compiler:**
- GCC | Clang | MinGW | MSVC | Emscripten

**Build Toolchain:**
- [`build2`][build2]

<!-- **Automatically handled Dependencies:** -->
<!-- **Manually handled Requirements:** -->

<!-- ## Introduction -->

## Getting Started
<!-- ## Build, Test, and Install -->

`sandbox` relies on [the `build2` toolchain][build2], and in order to use it, you should ensure that the toolchain is installed on your system.
To begin, we suggest attempting to acquire it from your system's package manager.
In the event that the package is not officially available through your system, the [The `build2` Installation Manual][build2-install] offers straightforward instructions for building the toolchain from source for various target configurations.
Moreover, it is advisable, though not mandatory, to become acquainted with the [toolchain's documentation][build2-docs].
You can start by exploring [The `build2` Toolchain Introduction][build2-intro] and, if desired, delve into [The `build2` Build System Manual][build2-build-system].

<!-- ### Usage in `build2`-Based Projects

1. Add this repository to the `repositories.manifest` of your `build2` project.

```
:
role: prerequisite
location: https://github.com/ensketch/sandbox.git
```

2. State the package dependency with an [optional version constraint](https://build2.org/bpkg/doc/build2-package-manager-manual.xhtml#package-version-constraint) in the `manifest`s of all `build2` packages in your project that shall use this library.

```
depends: ensketch-sandbox ^0.1.0
```

3. Import the library in each `buildfile` that contains at least one target that depends on the library and declare it as a dependency.

```
import sandbox = ensketch-sandbox%lib{ensketch-sandbox}
exe{myexe}: {hxx cxx}{**} $sandbox
```
 -->

### Installation and Usage in Projects not Based on `build2`

*How to make a `build2` package available on your system for projects that do not use `build2`?*

The instructions provided here are extracted from [The `build2` Toolchain Introduction: Package Consumption][build2-intro-consumption], focusing on the essential steps. We strongly recommend that you peruse the manual section for a more comprehensive understanding of advanced consumption strategies and workflows.
Upon completing the installation, you can seamlessly import the library into your project by utilizing the provided `pkg-config` file or, if preferred, manually specify the include and library paths to the compiler.
For further insights into using unpackaged dependencies, even in build strategies that do not rely on `build2`, refer to [The `build2` Toolchain Introduction: Using Unpackaged Dependencies][build2-intro-unpackaged-dependencies].

1. If you have not already done this, create a suitable build configuration for C/C++-based `build2` projects. You can use this one for all your needed `build2` packages by starting from step `2`.

```
bpkg create -d build2-packages cc \
  config.cxx=g++ \
  config.cc.coptions="-O3 -march=native" \
  config.install.root=/usr/local \
  config.install.sudo=sudo
```

2. Fetch the repository's packages and build them.

```
bpkg build https://github.com/ensketch/sandbox.git
```

3. Install the built packages.

```
bpkg install ensketch-sandbox
```

4. Optionally, uninstall the built packages when you do not need them any longer.

```
bpkg uninstall ensketch-sandbox
```

5. To upgrade the package, fetch all new information and uninstall the older version. Afterwards, build the new version and install it.

```
bpkg fetch
bpkg status

bpkg uninstall ensketch-sandbox
bpkg build ensketch-sandbox
bpkg install ensketch-sandbox
```

### Development Setup

The essential steps for preparing this repository for development are sourced from [the `build2` documentation][build2-docs] and are presented here in a concise format.
Familiarity with the fundamental usage of the `build2` toolchain is a prerequisite. For a more comprehensive grasp of employing `build2` in your development tasks, please refer to [The build2 Toolchain Introduction][build2-intro].

1. It is strongly advised to establish a dedicated development directory that will serve as the home for all project-specific build files. Clone the repository into this designated folder and run the script to initialize the development environment.

```
mkdir ensketch && cd ensketch
git clone --recurse https://github.com/ensketch/sandbox.git
cd sandbox
.develop/init.sh
```

2. Initialize the project using `bdep` by creating the initial build configuration. By default, this configuration will be automatically placed within the designated development folder, unless you manually specify the configuration directory. Below, you'll find an example command to create a build configuration named `gcc-release` with support for installation and distribution. Following the configuration creation, the command proceeds to initialize the project.

```
bdep init -C @gcc-release cc \
  config.cxx=g++ \
  config.cxx.coptions="-O3 -march=native" \
  config.install.root=../.install \
  config.dist.root=../.dist
```

3. Let's dive into development. Below are key steps and common commands to guide you in the process.

  + Use `bdep` to print the status of project packages and their immediate dependencies.

```
bdep status -i
```

  + Use `b` to build and test the source code.

```
b                      # Build all the code.
b test                 # Only build and run the tests.
b {clean update}       # Clean build all the code.
b {clean update test}  # Clean build all the code and run the tests.
```

  + Similarly, to build and test the source in a specific build configuration with `bdep`, run the following commands.

```
bdep update @gcc-release
bdep test @gcc-release
bdep clean @gcc-release
```

  + Use `b` to test the installation of the project. In the example configuration `gcc-release` created above, the installation directory `.install` is part of the development folder.

```
b install
b uninstall
```

  + Use `b` to test the distribution of the project's packages. In the example configuration `gcc-release` created above, the directory for distributions `.dist` is part of the development folder.

```
b dist: ensketch-sandbox
```

  + After thoroughly testing the aforementioned steps, make a commit and leverage [`build2`'s standard CI service][ci.cppget.org] to simultaneously build and test the project across a [range of build configurations][ci.cppget.org-build-configs].

```
bdep ci
```

## Configuration

`sandbox` does not include any publicly accessible configuration options.

## Documentation

Currently, all project documentation is consolidated within this `README.md` file.
There may be considerations to extract longer sections of the documentation in the future.

### Usage, Examples, and Tutorials

Currently, there are no examples and/or tutorials available.

### FAQs and HOWTOs

Currently, there are no FAQs and/or HOWTO entries available.

### API Reference

At present, we regret to inform you that there is no API reference documentation available, primarily due to the absence of a stable and automated generation mechanism for modern C++ libraries.
However, we encourage you to explore the source files for an extensive catalog of namespaces, classes, functions, and variables.

Our source files are organized into interface and implementation units.
In the interface units, you'll find a substantial amount of documentation comments that elucidate the behavior and API of functions and classes, designed to enhance your understanding as a library user.
In the implementation units, only essential comments are provided, offering additional insights or highlighting specific implementation strategies.

### Background and Discussions

Currently, there are no background documentation or discussions available.

## Roadmap

Currently, there is no roadmap available.

## Changelog

No changelog is currently available as no versions have been released yet.

## Contributing

`sandbox` is an open-source project, and we highly appreciate and value contributions from individuals like you.

- If you encounter any bugs or issues, we kindly request that you use our [GitHub issue tracking page][project-issues] to report them.
- If you have innovative ideas or feel that certain capabilities are missing, please don't hesitate to request a new feature.
- Should you discover gaps in the documentation, we encourage you to ask a question or propose enhancements.
- If you aspire to participate in development, consider forking the repository and submitting a pull request with your proposed changes.

Prior to submitting a pull request or contributing, please consult our [contribution guidelines][ensketch-contributing] and [code of conduct][ensketch-code-of-conduct] for comprehensive information.

## Code of Conduct

All contributors are expected to adhere to [our established code of conduct][ensketch-code-of-conduct].

## Contact

If you have any questions or comments regarding `sandbox`, please don't hesitate to reach out to us at ensketch@mailbox.org.
If you come across any bugs or encounter issues, we kindly request that you make use of our [GitHub issue tracking page][project-issues].
Additionally, if you are incorporating `sandbox` into your projects, we'd greatly appreciate it if you could drop us a brief message.
It would be immensely helpful if you could introduce yourself and provide insights into your use case.
This information plays a crucial role in justifying the time and effort we invest in maintaining this project.

## Copyright and License

The copyright for the code is held by the contributors of the code.
The revision history in the version control system is the primary source of authorship information for copyright purposes.
Please see individual source files for appropriate copyright notices.
For more information, see the file [`AUTHORS.md`](AUTHORS.md).

`sandbox` is free software, distributed under the terms of the GNU General
Public License as published by the Free Software Foundation,
version 3 of the License (or any later version).  For more information,
see the [GNU General Public License][GPLv3] or the file [`COPYING.md`](COPYING.md).

`sandbox` is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

Copyright years on `sandbox` source files may be listed using range notation, e.g., 1987-2012, indicating that every year in the range, inclusive, is a copyrightable year that could otherwise be listed individually.

Copying and distribution of this file, with or without modification, are permitted in any medium without royalty provided the copyright notice and this notice are preserved.
This file is offered as-is, without any warranty.

## References and Other Resources

- [Ensketch's Default Contributing Guidelines][ensketch-contributing]
- [Ensketch's Default Code of Conduct][ensketch-code-of-conduct]

- [`build2` | C/C++ Build Toolchain][build2]
<!-- - [`cppget.org` | Standard `build2` Package Repository][cppget.org] -->
<!-- - [`ci.cppget.org` | Standard `build2` CI Service][ci.cppget.org] -->
<!-- - [`build2` | Installation][build2-install] -->
<!-- - [`build2` | Documentation][build2-docs] -->
<!-- - [The `build2` Toolchain Introduction][build2-intro] -->
<!-- - [The `build2` Build System][build2-build-system] -->
<!-- - [The `build2` Package Manager][build2-package-manager] -->
<!-- - [The `build2` Toolchain Introduction: Package Consumption][build2-intro-consumption] -->
<!-- - [The `build2` Toolchain Introduction: Using Unpackaged Dependencies][build2-intro-unpackaged-dependencies] -->

- [GNU Licenses][GNU-licenses]
<!-- - [GNU General Public License Version 3][GPLv3] -->

[manifest]: ensketch-sandbox/manifest (ensketch-sandbox build2 Package Manifest)
[project-issues]: https://github.com/ensketch/sandbox/issues (Project Issues)

[ensketch-code-of-conduct]: https://github.com/ensketch/.github/blob/main/CODE_OF_CONDUCT.md (Ensketch's Default Code of Conduct)
[ensketch-contributing]: https://github.com/ensketch/.github/blob/main/CONTRIBUTING.md (Ensketch's Default Contributing Guidelines)

[build2]: https://build2.org (build2 | C/C++ Build Toolchain)
[build2-install]: https://build2.org/install.xhtml (build2 | Installation)
[build2-docs]: https://build2.org/doc.xhtml (build2 | Documentation)
[build2-intro]: https://build2.org/build2-toolchain/doc/build2-toolchain-intro.xhtml (The build2 Toolchain Introduction)
[build2-build-system]: https://build2.org/build2/doc/build2-build-system-manual.xhtml (The build2 Build System)
[build2-package-manager]: https://build2.org/bpkg/doc/build2-package-manager-manual.xhtml (The build2 Package Manager)
[build2-intro-consumption]: https://build2.org/build2-toolchain/doc/build2-toolchain-intro.xhtml#guide-consume-pkg (The build2 Toolchain Introduction: Package Consumption)
[build2-intro-unpackaged-dependencies]: https://build2.org/build2-toolchain/doc/build2-toolchain-intro.xhtml#guide-unpackaged-deps (The build2 Toolchain Introduction: Using Unpackaged Dependencies)

[cppget.org]: https://cppget.org/ (Standard build2 Package Repositories)
[ci.cppget.org]: https://ci.cppget.org/ (Standard build2 CI Service)
[ci.cppget.org-build-configs]: https://ci.cppget.org/?build-configs (Build Configurations in Standard build2 CI Service)

[GNU-licenses]: https://www.gnu.org/licenses/ (GNU Licenses)
[GPLv3]: https://www.gnu.org/licenses/gplv3.html (GNU General Public License Version 3)
