# 📘 Cscope + Ctags + Vim Navigation Notes

## 🔧 1. Install Required Tools

``` bash
sudo apt update
sudo apt install cscope universal-ctags
```

------------------------------------------------------------------------

# 📂 2. Generate Databases (run in project root)

## Step 1 --- Create file list for cscope

``` bash
find . -name "*.[ch]" > cscope.files
```

For C++ projects:

``` bash
find . -name "*.[ch]" -o -name "*.cpp" > cscope.files
```

------------------------------------------------------------------------

## Step 2 --- Build cscope database

``` bash
cscope -Rbq
```

Generated files:

    cscope.out
    cscope.in.out
    cscope.po.out

------------------------------------------------------------------------

## Step 3 --- Build tags database

``` bash
ctags -R .
```

Generated file:

    tags

------------------------------------------------------------------------

# 🖥️ 3. Using Cscope Terminal UI

Start UI:

``` bash
cscope
```

## Navigation Keys

  Key      Action
  -------- ----------------------
  ↑ ↓      Move between options
  TAB      Move to input box
  ENTER    Search / Open result
  Ctrl+B   Go back
  q        Exit pager

------------------------------------------------------------------------

# 🔎 4. Useful Searches in Cscope

  Option                              Use
  ----------------------------------- -------------------------------
  Find this C symbol                  Search functions/structs/vars
  Find global definition              Jump to function definition
  Functions calling this function     Who calls it
  Functions called by this function   What it calls
  Find this text string               Fast grep
  Find this file                      Open file quickly

------------------------------------------------------------------------

# 🧠 5. Using Vim with Cscope + Ctags

Open file:

``` bash
vim main.c
```

Add cscope DB:

``` vim
:cs add cscope.out
```

------------------------------------------------------------------------

## 🎯 Jump Commands in Vim

  Command           Meaning
  ----------------- ----------------------------------
  Ctrl+\]           Jump to definition (tags/cscope)
  Ctrl+T            Go back to previous location
  :cs find g func   Go to global definition
  :cs find c func   Show callers
  :cs find d func   Show functions it calls
  :cs find t text   Search text
  gd                Go to local definition
  gD                Go to global definition

------------------------------------------------------------------------

# ⚙️ 6. Recommended `.vimrc` Setup

Add this to `~/.vimrc`:

``` vim
set tags=./tags;,tags

if filereadable("cscope.out")
    cs add cscope.out
endif

set cscopequickfix=s-,c-,d-,i-,t-,e-

" Make Ctrl+] use cscope
nnoremap <C-]> :cs find g <C-R>=expand("<cword>")<CR><CR>
```

------------------------------------------------------------------------

# 🔁 7. Rebuild Databases After Code Change

``` bash
find . -name "*.[ch]" > cscope.files
cscope -Rbq
ctags -R .
```

------------------------------------------------------------------------

# 🚀 8. Typical Workflow

1.  Build DB once\
2.  Open Vim\
3.  Ctrl+\] → jump to function\
4.  Ctrl+T → go back\
5.  :cs find c func → see call chain\
6.  Trace execution across files easily

------------------------------------------------------------------------

# ✅ Done

You now have terminal-based IDE navigation for large C projects (DPDK,
kernel, drivers, etc.).
