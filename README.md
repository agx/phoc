Phone compositor
================

[wlroots][1] based Phone compositor as used on the Librem5.

Phoc is pronounced like the English word fog.

## Building

We use the meson (and thereby Ninja) build system for phosh.  The quickest
way to get going is to do the following:

    meson . _build
    ninja -C _build
    ninja -C _build install

This assumes you have wlroots installed on your system. If you don't have that
and/or want to build from source run:

    git submodule update --init
    meson . _build
    ninja -C _build

This will fetch a matching version of wlroots and build that as well.

## Running

To run from the source tree use

    _build/run

[1]: https://github.com/swaywm/wlroots
