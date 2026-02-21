## TraceMaD

Small C++/SDL3 2D ray‑tracing / light‑casting demo.

<p align="center">
  <img src="assets/Tracemad.gif" width="500px" />
</p>

### Run (Windows)
- Keep `main.exe` and `SDL3.dll` in the project root.
- Run from a terminal:

```bash
.\main.exe
```

### Build (g++)
- Requires a C++17+ compiler and the SDL3 headers/libs in `include/` and `lib/`.

```bash
g++ -std=c++17 -Iinclude -Llib src/main.cpp -lSDL3 -o main.exe
```

### Controls
- **Move mouse**: moves the light source
- **Close window**: exit


