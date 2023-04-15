## Name

**follow** - watch-like program with pager capabilities

## Synopsis

`follow.py [--interval SECS] [--shell] command [arguments...]`

## Description

**follow** periodically executes the provided command and shows its output on the terminal. It is intended for commands producing a large amount of output by provided a substen of the `less` commands for navigation.

## Options

<dl>
<dt>--interval SECS, -i SECS</dt>
<dd>Update intervals for the command, in seconds.</dd>
<dt>--shell, -s</dt>
<dd>Execute the command through a shell, rather than directly.</dd>
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
