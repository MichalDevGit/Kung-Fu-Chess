# Instructions for AI assistants working in this repo

Before exploring the source tree, doing architecture-level reasoning, or answering
questions about how this codebase is built, **read [ARCHITECTURE.md](ARCHITECTURE.md)
first.** It documents the module layout, dependency directions, the domain model, the
rules/real-time engine, the UI/rendering layer, the build system, the test setup, and
a list of known gaps/bugs (including a couple of real ones worth knowing about before
you touch related code). It exists specifically so you don't have to re-read all of
`src/` from scratch every session.

Use it as your map, not as a substitute for reading the actual file you're about to
edit — always open and read the real source file before changing it, since the
summary can drift out of date. If you find that a change you're making makes a
section of `ARCHITECTURE.md` wrong or stale, update that section as part of the same
change (this keeps the document trustworthy for the next session).

Do not read through `lib/include/opencv2/**` (vendored OpenCV headers) or
`assets/**` sprite/image files as part of general exploration — they're large,
third-party/binary, and not relevant to understanding this project's own code.
