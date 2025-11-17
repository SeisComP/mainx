scmagcopy is a SeisComP module which copies a
:ref:`api-python-datamodel-magnitude` of a specific type, e.g., M(USGS), to a
:ref:`api-python-datamodel-origin` that has been associated to an
:ref:`api-python-datamodel-event`.

Processing steps performed:

* Listen for new origins associated to an event
  (:ref:`api-python-datamodel-originreference`).
* Check if the referenced origin already contains a magnitude of the configured
  type and stop here if this is the case.
* Iterate through all origins and magnitudes of the corresponding event.
* Copy the latest magnitude of the configured type to the origin referenced by
  the new origin reference just received.
* Send the magnitude copy to the messaging system.
* Let :ref:`scevent` decide whether the new magnitude becomes preferred.

