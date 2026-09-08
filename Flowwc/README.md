# flowwc

`flowwc.flow` is a real Flowmini application. Its line, word, and byte
counting state machine runs in Flowmini; the filesystem provider only
materializes a safe `list<int>` value from a path.

Run a file-path invocation with:

```bash
flowmini Flowwc/src/flowwc.flow file.txt
```

The application also keeps a stdin regression source:

```bash
flowmini Flowwc/src/flowwc_stdin.flow < file.txt
```

Words are maximal non-whitespace byte sequences. Bytes are the exact number
of bytes consumed from the provider, so UTF-8 input contributes one count per
encoded byte.
