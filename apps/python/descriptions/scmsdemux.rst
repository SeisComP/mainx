scmsdemux demultiplexes :term:`miniSEED` records found in the given source
writing them into separate new files. The source can be files or stdin. One
file per stream is generated. File names are derived from the stream code and
the begin time of the records.

Examples:

* Demultiplex the miniSEED records contained in :file:`data.mseed` and
  additionally print the names of created files to stderr

  .. code-block:: sh

     scmsdemux -v data.mseed

* Demultiplex the miniSEED records received from stdin

  .. code-block:: sh

     scmssort -u -E data.mseed | scmsdemux -
