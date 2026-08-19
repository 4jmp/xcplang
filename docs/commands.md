# command helpers

Import `stdlib/commands.xcp` to get 200 fixed helpers for system, files, builds, and project data.

```xcp
import "stdlib/commands.xcp"
print(system_pwd_now())
print(file_list_show())
print(build_make_check())
```

Each helper uses `run_bash` with a fixed command and no user shell input. The names describe the command and the result view.
