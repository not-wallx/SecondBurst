```markdown
# SecondBurst

A simple Windows desktop utility application written in pure C using Win32 API.

## Features
- 400x300 pixel window
- Custom GDI rendering with rounded rectangles
- Minimalist black, white, and blue color scheme
- No external dependencies

## Building

### Method 1: MinGW/GCC
```bash
gcc src/*.c -o SecondBurst.exe -lgdi32 -luser32 -lkernel32 -mwindows
```

### Method 2: CMake
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Method 3: Visual Studio
Open `SecondBurst.sln` in Visual Studio and build the solution (Ctrl+Shift+B).

## Requirements
- Windows 7 or later
- MinGW-w64, CMake, or Visual Studio 2015+

## License
Public Domain
```
