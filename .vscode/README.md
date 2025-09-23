# VSCode Configuration

This directory contains pre-configured VSCode settings optimized for this project's multi-configuration build system.

## What's Included

- **Build Tasks**: Quick access to all 4 build configurations (debug/release × threaded/single)
- **Debug Configurations**: Pre-configured debugging for both single-threaded and multi-threaded builds
- **IntelliSense Settings**: Proper include paths and conditional compilation for threading support
- **CMake Integration**: Build variants and workspace settings

## Usage

These configurations should work out-of-the-box for most users. Key features:

- **Ctrl+Shift+B**: Quick build (defaults to Release Threaded)
- **F5**: Debug with automatic build (recommended: Debug Single-threaded for easier stepping)
- **Status Bar**: CMake variant selector for build configuration switching

## Customization

Feel free to modify these settings for your preferences:

- **Keybindings**: Add custom shortcuts in your user settings
- **Debug Settings**: Adjust debugger preferences in `launch.json`
- **Build Variants**: Modify `settings.json` CMake variants section

## Alternative Setup

If you prefer your own VSCode configuration, these files can be safely ignored or deleted. The build system works independently via:

```bash
./quick_build.sh [debug|release] [threads|nothreads]
./build_all.sh
```