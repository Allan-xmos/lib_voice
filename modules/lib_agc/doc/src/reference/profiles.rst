.. _agc_profiles:

AGC Pre-Defined Profiles and Parameters
=======================================

Three pre-defined profiles are provided in `agc_profiles.h` to configure the AGC for different applications:

.. doxygengroup:: agc_profiles

These profiles can be used to configure the AGC instance by passing them to the
:c:func:`agc_init` function.

AGC Parameters
**************

The key AGC parameters are highlighted below:

.. doxygenstruct:: agc_config_t
   :members-only:
   :members: adapt, vnr_threshold, gain, max_gain, upper_threshold, lower_threshold,
      soft_clipping, lc_enabled, lc_near_delta, lc_near_delta_far_active
   :no-link:

Other AGC parameters are described in the `agc_profiles.h` header file,
and are described in detail in :c:struct:`agc_config_t`.
