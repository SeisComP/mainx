#!/usr/bin/env seiscomp-python
# -*- coding: utf-8 -*-
"""
scmagimport - Import a foreign magnitude from a SCML event file into SeisComP.

Processing steps performed:

* Extract the preferred origin and magnitude from the SCML event file.
* Check if the foreign magnitude is of a specific type, e.g., M(USGS).
* Use :ref:`scevent` to associate the origin to an SeisComP event. In this step the
  foreign origin is send via HTTP POST to the :ref:`scevent` REST API
  `/api/1/try-to-associate` which responds with an event ID if the association
  was successful.
* Retrieve the event and the preferred origin from the local SeisComP system.
* Copy the foreign magnitude to the preferred origin.
* Send the magnitude copy via a notifier message to :ref:`scmaster`.
* Let :ref:`scevent` decide whether the new magnitude becomes preferred.

Usage: scmagimport [OPTIONS]
"""

import sys

from io import BytesIO

import requests

from seiscomp import client, datamodel, io as scio, logging


class Sink(scio.ExportSink):
    def __init__(self, buf):
        super().__init__()
        self.buf = buf

    def write(self, data: bytes):
        self.buf.write(data)
        return len(data)


class ExportSinkBuffer(BytesIO):
    """
    Bytes buffer derived from BytesIO while implementing the seiscomp.io.ExportSink
    interface.
    """

    def __init__(self):
        super().__init__()
        self.sink = Sink(self)

    def __str__(self):
        return self.getvalue().decode("utf-8", "replace")

    def __bytes__(self):
        return self.getvalue()


class MagImporter(client.Application):
    def __init__(self, argc, argv):
        super().__init__(argc, argv)
        self.setMessagingEnabled(True)
        self.setDatabaseEnabled(True, True)
        self.setPrimaryMessagingGroup("MAGNITUDE")
        self.setLoggingToStdErr(True)

        self.inputFile = None
        self.assocAPI = "http://localhost:18182/api/1/try-to-associate"
        self.magType = "M(USGS)"

        self.apiTimeout = 3

    def createCommandLineDescription(self):
        super().createCommandLineDescription()

        self.commandline().addGroup("Processing")
        self.commandline().addStringOption(
            "Processing",
            "input,i",
            "SeisComP XML file containing an event with a preferred origin and "
            "magnitude to import. Use '-' to read from stdin instead of file.",
        )
        self.commandline().addStringOption(
            "Processing",
            "associate-api,a",
            "scevent API URL for event association, default: {self.assocAPI}.",
        )
        self.commandline().addStringOption(
            "Processing",
            "mag-type",
            "Magnitude type to use for the imported magnitude, default: "
            f"{self.magType}.",
        )

    def validateParameters(self) -> bool:
        if not super().validateParameters():
            return False

        try:
            self.inputFile = self.commandline().optionString("input")
        except RuntimeError:
            logging.error("Parameter 'input' not given")
            return False

        try:
            self.assocAPI = self.commandline().optionString("associate-api")
        except RuntimeError:
            pass

        try:
            self.magType = self.commandline().optionString("mag-type")
        except RuntimeError:
            pass

        return True

    def printUsage(self):
        # Module specific information
        print(__doc__)

        # Parameters inherited from base class
        super().printUsage()

        # Examples
        print(
            """
Examples:
=========

Read preferred origin and magnitude from SCML file
    scmagimport -i /path/to/seiscomp_event.xml

Use xalan and a XSLT file to convert QuakeML file to SCML and pass it to the importer on stdin
    xalan -in /path/to/quakeml_event.xml -xsl $SEISCOMP_ROOT/share/xml/0.13/quakeml__sc3ml_0.13.xsl | scmagimport -i -
"""
        )

    @staticmethod
    def readEventParameters(fileName: str) -> datamodel.EventParameters:
        ar = scio.XMLArchive()
        if not ar.open(fileName):
            raise OSError("Could not read SeisComP XML")
        obj = ar.readObject()
        if not obj:
            raise ValueError("No object found")
        ep = datamodel.EventParameters.Cast(obj)
        if not ep:
            raise ValueError("XML object not of type EventParameters")

        return ep

    @staticmethod
    def readEvent(ep: datamodel.EventParameters) -> datamodel.Event:
        if not ep.eventCount():
            raise ValueError("No event found")

        if ep.eventCount() > 1:
            logging.warning("More than one event found in input XML, using first one")

        return ep.event(0)

    @staticmethod
    def readPrefOriginAndMag(
        event: datamodel.Event,
    ) -> (datamodel.Origin, datamodel.Magnitude):
        # Extract preferred origin and magnitude
        origin = datamodel.Origin.Find(event.preferredOriginID())
        mag = datamodel.Magnitude.Find(event.preferredMagnitudeID())

        if not origin:
            raise ValueError(f"Preferred origin not found: {origin.publicID()}")
        if not mag:
            raise ValueError(f"Preferred magnitude not found: {mag.publicID()}")

        try:
            depth = origin.depth().value()
        except ValueError:
            depth = "-"

        logging.debug(
            f"""Imported event
  eventID:     {event.publicID()}
  origin:
    time:      {origin.time().value().iso()}
    epicenter: {origin.latitude().value()}/{origin.longitude().value()}
    depth:     {depth}
  magnitude:   {mag.magnitude().value()} ({mag.type()})"""
        )

        return origin, mag

    @staticmethod
    def objectToBuffer(
        obj: datamodel.Object, expName: str = "trunk"
    ) -> ExportSinkBuffer:
        exp = scio.Exporter.Create(expName)
        if not exp:
            raise OSError(f"Could not create exporter: {expName}")

        buf = ExportSinkBuffer()
        exp.write(buf.sink, obj)
        return buf

    @staticmethod
    def serializeOrigin(
        origin: datamodel.Origin, expName: str = "trunk"
    ) -> ExportSinkBuffer:
        oldParent = origin.parent()
        if oldParent:
            origin.detach()

        ep = datamodel.EventParameters()
        ep.add(origin)
        origin_buffer = MagImporter.objectToBuffer(ep, expName)

        if oldParent:
            origin.detach()
            origin.attachTo(oldParent)

        return origin_buffer

    def associateOrigin(self, origin: datamodel.Origin) -> str:
        origin_buffer = MagImporter.serializeOrigin(origin)

        headers = {"Content-Type": "text/xml"}

        resp = requests.post(
            self.assocAPI,
            data=bytes(origin_buffer),
            headers=headers,
            timeout=self.apiTimeout,
        )

        resp.raise_for_status()

        if resp.status_code == 204:
            raise ValueError("No matching event found")

        if resp.status_code != 200:
            raise ValueError(f"Got response code {resp.status_code}, expected 200")

        eventID = resp.text[:200]
        if not eventID.isalnum():
            raise ValueError(
                "Association API response contains non alpha numeric characters: "
                f"{eventID}"
            )

        return eventID

    def loadPreferredOrigin(self, eventID) -> datamodel.Origin:
        event = self.query().getEventByPublicID(eventID)
        if not event:
            raise ValueError(f"Could not load event {eventID} from database")

        prefOriginID = event.preferredOriginID()
        if not prefOriginID:
            raise ValueError(f"Event {eventID} does not reference a preferred origin")

        origin = datamodel.Origin.Cast(
            self.query().getObject(datamodel.Origin.TypeInfo(), prefOriginID)
        )
        if not origin:
            raise ValueError(
                f"Could not load preferred origin of event {eventID}: {prefOriginID}"
            )

        self.query().loadMagnitudes(origin)

        return origin

    def run(self):
        # Read external SeisComP EventParameters object from XML file
        try:
            # datamodel.PublicObject(False)
            inp_ep = self.readEventParameters(self.inputFile)
            inp_event = self.readEvent(inp_ep)
            inp_origin, inp_mag = self.readPrefOriginAndMag(inp_event)
            # datamodel.PublicObject(True)
        except BaseException as e:
            logging.error(f"Invalid input file {self.inputFile}: {e}")
            return False

        # Associate origin to an SeisComP event using scevent REST API
        try:
            sc_eventID = self.associateOrigin(inp_origin)
        except BaseException as e:
            logging.error(f"Event association failed: {e}")
            return False

        logging.info(f"Imported origin associated to SeisComP event {sc_eventID}")

        # Add the imported magnitude
        try:
            sc_origin = self.loadPreferredOrigin(sc_eventID)
            if self.magType:
                inp_mag.setType(self.magType)
            inp_mag.detach()

            datamodel.Notifier.SetEnabled(True)

            # Check if mag type already exists
            sc_mag = None
            for iMag in range(sc_origin.magnitudeCount()):
                mag = sc_origin.magnitude(iMag)
                if mag.type() == self.magType:
                    sc_mag = mag
                    break
            if sc_mag:
                logging.warning(
                    "Preferred origin already contains magnitude of type "
                    f"{self.magType}"
                )

            logging.info(
                f"Adding magnitude {inp_mag.magnitude().value()} ({inp_mag.type()}) "
                f"to preferredOrigin of event {sc_eventID}"
            )
            sc_origin.add(inp_mag)
            msg = datamodel.Notifier.GetMessage()
            datamodel.Notifier.SetEnabled(False)
            self.connection().send(msg)
            logging.debug("NotifierMessage successfully sent")

        except BaseException as e:
            logging.error(f"Magnitude processing failed: {e}")
            return False

        return True


def main():
    app = MagImporter(len(sys.argv), sys.argv)
    return app()


if __name__ == "__main__":
    sys.exit(main())
