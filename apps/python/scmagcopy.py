#!/usr/bin/env seiscomp-python
# -*- coding: utf-8 -*-
"""
scmagcopy - SeisComP module copying magnitudes to new origin references.

Processing steps performed:

 - Listen for new origins associated to an event (OriginReference).
 - Check if the referenced origin already contains a magnitude of the configured
   type and stop here if this is the case.
 - Iterate through all origins and magnitudes of the corresponding event.
 - Copy the latest magnitude of the configured type to the origin referenced by
   the new origin reference just received.
 - Send the magnitude copy to the messaging system.
 - Let scevent decide whether the new magnitude becomes preferred.

Usage: scmagcopy [OPTIONS]
"""

import sys

from seiscomp import client, core, datamodel, logging


class MagnitudeCopy(client.Application):
    def __init__(self, argc, argv):
        super().__init__(argc, argv)
        self.setMessagingEnabled(True)
        self.setDatabaseEnabled(True, True)
        self.addMessagingSubscription("EVENT")
        self.addMessagingSubscription("LOCATION")
        self.addMessagingSubscription("MAGNITUDE")
        self.setPrimaryMessagingGroup("MAGNITUDE")

        self.bufferSize = 4 * 3600
        self.poCache = datamodel.PublicObjectTimeSpanBuffer()
        self.magType = "M(USGS)"

    def createCommandLineDescription(self):
        super().createCommandLineDescription()

        self.commandline().addStringOption(
            "Processing",
            "mag-type",
            f"Magnitude type to copy if present, default: {self.magType}.",
        )

    def validateParameters(self) -> bool:
        if not super().validateParameters():
            return False

        try:
            self.magType = self.commandline().optionString("mag-type")
        except RuntimeError:
            pass

        return True

    def initConfiguration(self):
        if not client.Application.initConfiguration(self):
            return False

        try:
            self.bufferSize = self.configGetInt("bufferSize")
        except RuntimeError:
            pass

        try:
            self.magType = self.configGetString("magType")
        except RuntimeError:
            pass

        return True

    def printUsage(self):
        # Module specific information
        print(__doc__)

        # Parameters inherited from base class
        super().printUsage()

    def init(self):
        if not super().init():
            return False

        self.poCache.setTimeSpan(core.TimeSpan(self.bufferSize))
        self.poCache.setDatabaseArchive(self.query())

        return True

    @staticmethod
    def createdAfter(o1, o2):
        try:
            creationTime1 = o1.creationInfo().creationTime()
        except ValueError:
            creationTime1 = None

        try:
            creationTime2 = o2.creationInfo().creationTime()
        except ValueError:
            creationTime2 = None

        return creationTime2 and (not creationTime1 or creationTime2 > creationTime1)

    def copyMagnitude(self, magnitude, origin):
        # collect notifier
        wasEnabled = datamodel.Notifier.IsEnabled()
        datamodel.Notifier.SetEnabled(True)

        now = core.Time.UTC()

        # clone magnitude and generate new public ID
        magCopy = datamodel.Magnitude.Cast(magnitude.clone())
        datamodel.PublicObject.GenerateId(magCopy)

        # generate creation info referencing original magnitude
        ci = datamodel.CreationInfo()
        ci.setAgencyID(self.agencyID())
        ci.setAuthor(self.author())
        ci.setCreationTime(now)
        ci.setVersion(f"clone of {magnitude.publicID()}")
        magCopy.setCreationInfo(ci)

        # add magnitude to origin
        origin.add(magCopy)

        # send notifier
        msg = datamodel.Notifier.GetMessage()
        datamodel.Notifier.SetEnabled(wasEnabled)
        self.connection().send(msg)

        logging.info(
            f"Added magnitude {magCopy.publicID()} as copy of {magnitude.publicID()} "
            f"to origin {origin.publicID()}"
        )

    def processOriginReference(self, eventID, originReference):
        # load origin for reference
        originID = originReference.originID()
        logging.info(f"Processing origin reference {originID} of event {eventID}")
        origin = self.poCache.get(datamodel.Origin, originID)

        if not origin:
            logging.error(f"Could not load origin {originID}")
            return

        if not origin.magnitudeCount():
            self.query().loadMagnitudes(origin)

        # check if origin already contains magnitude of desired type
        for iMag in range(origin.magnitudeCount()):
            mag = origin.magnitude(iMag)
            # print(f"origin.magnitude.type: {mag.type()}")
            if mag.type() == self.magType:
                logging.info(
                    f"Origin {originID} already contains desired magnitude of type "
                    f"{self.magType}"
                )
                return

        logging.debug(
            f"Origin {originID} does not contain magnitude of type {self.magType} yet"
        )

        # load event
        event = self.poCache.get(datamodel.Event, eventID)
        if not event:
            logging.error(f"Could not load event {eventID}")
            return

        if not event.originReferenceCount():
            self.query().loadOriginReferences(event)

        logging.debug(
            f"Searching magnitude in {event.originReferenceCount()} origins of event"
        )

        # search for latest magnitude of given type in all referenced origins
        latestMag = None
        for iOriginRef in range(event.originReferenceCount()):
            templateOriginID = event.originReference(iOriginRef).originID()
            print(f"testing oref: {templateOriginID}")
            if templateOriginID == originID:
                continue

            templateOrigin = self.poCache.get(datamodel.Origin, templateOriginID)
            if not templateOrigin:
                logging.warning(f"Could not load origin {templateOriginID}")
                continue

            if not templateOrigin.magnitudeCount():
                self.query().loadMagnitudes(templateOrigin)

            print(f"templateOrigin.magnitudeCount(): {templateOrigin.magnitudeCount()}")

            for iMag in range(templateOrigin.magnitudeCount()):
                mag = templateOrigin.magnitude(iMag)
                print(f"  mag: {mag.type()}")
                if mag.type() == self.magType and (
                    not latestMag or self.createdAfter(latestMag, mag)
                ):
                    latestMag = mag

        if latestMag:
            self.copyMagnitude(latestMag, origin)
        else:
            logging.info(
                f"Event {eventID} does not contain an origin with a magnitude of type "
                f"{self.magType}"
            )

    def addObject(self, parentID, scobject):  # pylint: disable=W0237
        # cache event objects
        event = datamodel.Event.Cast(scobject)
        if event:
            logging.debug(f"Caching event {event.publicID()}")
            self.poCache.feed(event)
            return

        # cache origin objects
        origin = datamodel.Origin.Cast(scobject)
        if origin:
            logging.debug(f"Caching origin {origin.publicID()}")
            self.poCache.feed(origin)
            return

        # cache magnitude objects
        mag = datamodel.Magnitude.Cast(scobject)
        if mag:
            logging.debug(f"Caching magnitude {mag.publicID()}")
            self.poCache.feed(mag)
            return

        # process origin references added to events
        originReference = datamodel.OriginReference.Cast(scobject)
        if originReference:
            logging.debug(f"Caching originReference {originReference.originID()}")
            self.processOriginReference(parentID, originReference)


def main():
    app = MagnitudeCopy(len(sys.argv), sys.argv)
    return app()


if __name__ == "__main__":
    sys.exit(main())
