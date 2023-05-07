## Name

**follow** - paging through the output of a given command, which is periodically refreshed

## Synopsis

`follow [-n SECS|--interval SECS] [-s|--shell] [-t|--no-title] command [arguments...]`

`follow -h | --help`

`follow -v | --version`

## Description

**follow** is similar to `watch`, but provides paging capabilities. **follow** periodically executes the provided command and shows its output on the terminal. It is intended for commands producing a large amount of output by providing a subset of the `less` commands for navigation.

Two versions exist, which only differ in how the command is executed:
- A ready-to-use Python script (`follow.py`); the command is executed synchronously, so the interface freezes during its execution (particularly noticeable for commands that need some time to run)
- A C program that can be compiled using GNU Autotools: the command is executed in the background, and the interface remains responsive all the time

## Options

<dl>
<dt>-h, --help</dt>
<dd>Display the help message and exit.</dd>
<dt>-v, --version</dt>
<dd>Display version information and exit.</dd>
<dt>-n SECS, --interval SECS</dt>
<dd>Refresh the command every SECS seconds.</dd>
<dt>-s, --shell</dt>
<dd>Execute the command through a shell, rather than directly.</dd>
<dt>-t, --no-title</dt>
<dd>Don't show the header line.</dd>
</dl>

## Commands

**follow** understands a subset of the less commands for navigation through the command's output.
<dl>
<dt>LEFT ARROW</dt>
<dd>Move one column to the left</dd>
<dt>RIGHT ARROW</dt>
<dd>Move one column to the right</dd>
<dt>DOWN ARROW, e, j</dt>
<dd>Move one row downwards</dd>
<dt>UP ARROW, k, y</dt>
<dd>Move one row upwards</dd>
<dt>E, J</dt>
<dd>Move one row downwards (allow movement past the bottom of the output)</dd>
<dt>K, Y</dt>
<dd>Move one row upwards (allow movement past the top of the output)</dd>
<dt>SPACE, f</dt>
<dd>Move one screen height downwards</dd>
<dt>b</dt>
<dd>Move one screen height upwards</dd>
<dt>d</dt>
<dd>Move one half screen height downwards</dd>
<dt>u</dt>
<dd>Move one half screen height upwards</dd>
<dt>g</dt>
<dd>Go to top</dd>
<dt>G</dt>
<dd>Go to bottom</dd>
<dt>F</dt>
<dd>Remain at then bottom, even when the height changes (a repeat switches off that mode)</dd>
<dt>q, ^c</dt>
<dd>Exit the program.</dd>
</dl>
