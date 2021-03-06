------------------------------------------------------------------------------
This is essentially a placeholder for the next release note ...
------------------------------------------------------------------------------
(Copy the contents below to RELEASE_NOTE.md when making an official release.)

* New features
  + none    

* New optimization
  + none

* New Limitations
  + none

* Update configure options
  + none

* New constants
  + none

* New APIs
  + none

* New native HDF5 APIs
  + none

* API syntax changes
  + H5Pget/set_subfiling property type changed to int
    + Signature
      ```
        herr_t H5Pset_subfiling (hid_t plist, int nsubfiles);
        herr_t H5Pget_subfiling (hid_t plist, int *nsubfiles);
      ```
    + Allows setting number of subfiles in addition to enabling subfiling
    + Meaning of argument "nsubfiles"
      + 1 (H5VL_LOG_SUBFILING_OFF): Disable subfiling
      + Non-positive values (H5VL_LOG_SUBFILING_FILE_PER_NODE): One subfile per node
      + Positive value larger than 1: Create "nsubfiles" subfiles

* Run-time environment variables changes
  + The meaning of H5VL_LOG_NSUBFILES is changed to match argument "nsubfiles" in H5Pget/set_subfiling
    + See API syntax changes section for more information

* API semantics updates
  + Remove the restriction that does not allow user objects with a name starting with '_'

* New error code precedence
  + none

* Updated error strings
  + Return error if a file is opened multiple time
    + Logvol only allow one opened file handle at a time.

* New error code
  + none

* New I/O hint
  + none

* New run-time environment variables
  + none

* Build recipes
  + none

* Updated utility program
  + none

* Other updates:
  + none

* Bug fixes
  + Fix a bug when reading interleaving regions within a log entry
    + H5VL_log_dataset_readi_gen_rtypes produces an invalid file type due to interleaving read regions not being detected
  + Fix a bug in H5VL_log_link_create that uses the calling convention of an older VOL interface
  + Deduce internal attributes from attributes count in object info

* New example programs
  + none

* New programs for I/O benchmarks
  + none

* New test program
  + tests/testcases/multi_open
    + Test opening a second handle to the same file without closing the first one.

* Conformity with HDF5 library
  + none

* Discrepancy from HDF5 library
  + none

* Issues related to MPI library vendors:
  + none

* Issues related to Darshan library:
  + none

* Clarifications
  + none
