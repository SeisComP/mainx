scmagimport imports a foreign :ref:`api-python-datamodel-magnitude` from a
:term:`SCML` :ref:`api-python-datamodel-event` file into SeisComP.

Processing steps performed:

* Extract the preferred :ref:`api-python-datamodel-origin` and magnitude from
  the SCML event file.
* Check if the foreign magnitude is of a specific type, e.g., M(USGS).
* Use :ref:`scevent` to associate the origin to an SeisComP event. In this step the
  foreign origin is send via HTTP POST to the :ref:`scevent` REST API
  `/api/1/try-to-associate` which responds with an event ID if the association
  was successful.
* Retrieve the event and the preferred origin from the local SeisComP system.
* Copy the foreign magnitude to the preferred origin.
* Send the magnitude copy via a notifier message to :ref:`scmaster`.
* Let :ref:`scevent` decide whether the new magnitude becomes preferred.

Examples:

* Read preferred origin and magnitude from :term:`SCML` file

  .. code-block:: sh

     scmagimport -i /path/to/seiscomp_event.xml

* Use xalan and a XSLT file to convert :term:`QuakeML` file to :term:`SCML` and
  pass it to the importer on stdin

  .. code-block:: sh

     xalan -in /path/to/quakeml_event.xml -xsl $SEISCOMP_ROOT/share/xml/0.13/quakeml__sc3ml_0.13.xsl | scmagimport -i -
