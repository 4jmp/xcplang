# Modules

The module loader accepts a local file path with `.xcp` or a path without the extension.

```xcp
import "math_addon.xcp"
```

The loader resolves the path, reads the file, and sends its source to the runtime. Keep imported files inside the project or another trusted directory. Network imports are not supported.
