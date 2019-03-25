Building
========
For build instructions see the README.md

Pull requests
=============
Before filing a pull request run the tests:

```sh
ninja -C _build test
```

Use descriptive commit messages, see

   https://wiki.gnome.org/Git/CommitMessages

and check

   https://wiki.openstack.org/wiki/GitCommitMessages

for good examples.

Coding Style
============
The code base currently uses two coding styles

1. the one followed in [libhandy][1]
2. the [wlroots][2] one (for files taken from wlroots)

New files should use [libhandy][1] style. For other files use the style
prevalent in that file. It's also o.k. to use [libhandy][1] style for
completely new functions and structs in a file indented otherwise but don't mix
indentation within a single function or struct.

New public functions and structs should have a `phoc_` prefix.

[1]: https://source.puri.sm/Librem5/libhandy/blob/master/HACKING.md
[2]: https://github.com/swaywm/wlroots/blob/master/CONTRIBUTING.md
