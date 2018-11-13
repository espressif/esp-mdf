Mcommmon API
=============

``Mcommon`` (Mesh common) is a module shared by all ESP-MDF components, and it features the followings:

1. Memory Management: manages memory allocation and release, and can help find a memory leak;
2. Error Codes: checks the error codes of all the modules, and therefore can help find out what could possibly go wrong;
3. Event Loop: uses an identical function for all the modules to deal with an event.

.. ------------------------- Mconfig API Reference ---------------------------

.. _api-reference-mdf_mem:

Memory Management
------------------

.. include:: /_build/inc/mdf_mem.inc

.. _api-reference-mdf_err:

Error Codes
------------

.. include:: /_build/inc/mdf_err.inc

.. _api-reference-mdf_event_loop:

Event Loop
------------

.. include:: /_build/inc/mdf_event_loop.inc
