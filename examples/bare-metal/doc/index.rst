
####################
Example Applications
####################

Several examples are provided to demonstrate processing of audio using the audio processing algorithms individually as
well as put together in a pipeline.

Building Examples
==================

After configuring the CMake project and installing `xmos-ai-tools <https://pypi.org/project/xmos-ai-tools/>`_
into your python environment, all the examples can be built by using the ``xmake`` command within the build directory.
Individual examples can be built using ``xmake EXAMPLE_NAME``, where ``EXAMPLE_NAME`` is the example to build. 


.. toctree::
   :maxdepth: 3

   aec
   vnr
   pipeline
