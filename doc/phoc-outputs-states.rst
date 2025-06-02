.. _phoc-outputs-states(1):

===================
phoc-outputs-states
===================

-----------------------------------
Inspect phoc's saved outputs states
-----------------------------------

SYNOPSIS
--------
|   **phoc-outputs-states** [OPTIONS...]


DESCRIPTION
-----------

``phoc-outputs-states`` is a tool to list the saved outputs states
known to ``phoc``. These states are updated by phoc whenever the
output configuration changes and applied when ``phoc`` detects that
specific set of outputs.

Note that output configuration in ``phoc.ini`` always takes precedence
over saved outputs states.

OPTIONS
-------

``-h``, ``--help``
   Print help and exit
``-d``, ``--db FILENAME``
   Path to the saved outputs states database file. If omitted phoc's
   built in default will be used
``-r``, ``--raw``
   Use raw output, useful when processing the output in scripts
``--list``
   List the identifiers of the outputs states in the database. The returned
   identifiers are based on ``wlroots`` output descriptions
``-show=IDENTIFIER``
   Show the saved output state details for the given identifier

EXAMPLES
--------

Get current list of identifiers:

::

    $ phoc-outputs-states --list
    Identifiers
    -----------
    …
    '(null) (null) (DSI-1)'
    …

Get the details for the identifier ``(null) (null) (DSI-1)``:

::

    $ phoc-outputs-states --show '(null) (null) (DSI-1)'
    (null) (null) (DSI-1)
    ---------------------
      Enabled: yes
         Mode: 1080x2340@60000000.00
        Scale: 2.500000
    Transform: normal

See also
--------

``phoc(1)``
