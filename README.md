# latex-fast-compile

`latex-fast-compile` is a Linux helper for the following $\LaTeX$ workflow:

1. Precompile the "preamble" of a given `.tex` file (i.e. the part before `\csname endofdump \endcsname` or `\begin{document}`, whichever comes first)
2. Create a file descriptor watch using `inotify`, to bind an action whenever the given `.tex` file changes
3. The action: When the `.tex` file updates, compile the file, using the precompiled preamble to save time

## installation

A working copy of `pdflatex` must be available on your PATH (typically associated with packages named `texlive`/`texlive-latex`).

```bash
git clone https://github.com/latex-fast-compile
cd latex-fast-compile
make
sudo make install
```

If required, specify some `$PREFIX` to install to, for example,

```bash
PREFIX=~/.local/bin make install
```

## usage

```
latex-fast-compile main.tex
```

Use Control+D to skip over a $\LaTeX$ compilation error. Use Control+C to quit.

Any $\LaTeX$ in the preamble you'd prefer to regenerate? Separate
the "static" precompiled area and the "dynamic" compile-every-time area
using `\csname endofdump \endcsname`.

```latex
% "Static" preamble
\some_stuff_like_the_page_header
\csname endofdump \endcsname % Ends the "static" preamble
% "Dynamic" preamble
\stuff_that_changes_like_minitoc
\begin{document} % End of preamble, document starts
  ...
```

## how it works

The `inotify` Linux API is a somewhat old filesystem monitor
used to modify changes in a given directory. Here, we watch a
directory; if the filename changed matches the filename in the
argument, we fire off a compile command via `pdflatex`.

As for the $\LaTeX$ step, see this
[HowToTex (archived)](https://web.archive.org/web/20120524232434/http://www.howtotex.com/tips-tricks/faster-latex-part-iv-use-a-precompiled-preamble/)
article. We draw on this to follow this command workflow:

```sh
# In this example, the argument is "main.tex"
# 1. Precompile the preamble. This creates a preamble.fmt file
pdflatex -ini -jobname=preamble "&pdflatex" mylatexformat.ltx main.tex
# 2. Compile the main.tex file using the preamble
pdflatex -fmt preamble main.tex
```

You can read the code yourself in [`latex-fast-compile.c`](./latex-fast-compile.c).
It is well-commented and follows this pseudocode:

```
compile preamble
compile using preamble
loop forever:
  wait until edit
  compile using preamble
```

## license

GPLv3
