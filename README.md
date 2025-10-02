# CShell

## Overview
This project implements a basic Unix-like shell in C as an educational exercise. It demonstrates core concepts of process management, input/output handling, and command execution in a POSIX environment. The shell is not intended for production use and serves solely as a learning tool to explore shell internals.

## Features
The shell supports a range of fundamental shell behaviors, including:

### Command Execution and Navigation
- External command launching via `fork` and `exec` .
- Resolve enviornment variables.
- Directory navigation using `cd`, including `~` expansion for home directories.

### Command Sequencing and Control Flow
- Sequential execution of semicolon-separated (`;`) commands.
- Conditional chaining with `&&` and `||` based on exit statuses.
- Subshells enclosed in parentheses for isolated execution.
- Exit code negation using `!`.
- Line continuation with backslashes and support for multiline command lists.
- Shell process replacement via `exec` (applied twice for added reliability).

### I/O and Pipeline Management
- Pipeline operations using `|`.
- Input/output redirection with `[n]<`, `[n]>`, `[n]<>` , and `[n]>>` .
- Permission validation for redirection targets.

### Signal Handling
- SIGINT (Ctrl+C) handling to interrupt foreground processes and return to the shell prompt.
- At the input prompt, Ctrl+C refreshes the prompt and restarts the input loop, mimicking bash behavior without terminating the shell.

## Building and Running

### Prerequisites
- GCC or compatible C compiler.
- POSIX-compliant system (e.g., Linux, macOS).

### Compilation
```bash
make
```

This builds the executable `cshell` in the current directory.

### Usage
Launch the shell:
```bash
./cshell
```

Exit with `exit` or Ctrl+D.

## Notes
- Error handling focuses on usability, printing diagnostics to stderr.
- The implementation prioritizes clarity over optimization.

## References
- Stephen Brennan's [Write a Shell in C](https://brennan.io/2015/01/16/write-a-shell-in-c/)
-  Indradhanush Gupta's [Writing a UNIX shell](https://igupta.in/blog/writing-a-unix-shell-part-1/)
- Tokenrove's [Build-your-own-shell](https://github.com/tokenrove/build-your-own-shell)

## License
This project is for personal learning and is not licensed for redistribution or commercial use.
