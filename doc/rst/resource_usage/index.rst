|newpage|

.. _resource_usage_section:

**************
Resource usage
**************

Memory
======

:numref:`lib_voice_memory_usage` lists the memory requirements for the
``lib_voice`` DSP components. The estimates are provided as a guideline to help
audio pipeline developers assess the memory cost of including a component in
the pipeline.

.. note::

  The IC memory usage includes VNR memory usage as well, since VNR processing is part of IC. Stand-alone
  VNR memory usage is also computed, as shown in the VNR row of the table.

.. include:: ../../../tests/profile_memory/lib_voice_memory_table.rst

CPU
===

:numref:`lib_voice_mips_usage` lists the approximate CPU requirements in MIPS
for the ``lib_voice`` DSP components. The MIPS values are computed as the
worst-case processor cycles consumed by the component's ``process_frame``
function. All CPU estimates are based on the default configuration for each
feature. Alternate configurations may require more or less MIPS.

These estimates are provided as a guideline to help audio pipeline developers
assess the CPU cost of including a component in the pipeline.

.. note::

  The IC MIPS numbers include VNR processing that is part of IC. Stand-alone
  VNR processing numbers are also computed, as shown in the VNR row of the
  table.

.. include:: ../../../tests/profile_mips/lib_voice_mips_table.rst

