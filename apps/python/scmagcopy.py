#!/usr/bin/env seiscomp-python
# -*- coding: utf-8 -*-
"""
scmagcopy - SeisComP module copying magnitudes to new origin references.

Processing steps performed:

 1. Listen for new origins associated to an event (OriginReference).
 2. Check if the referenced origin already contains a magnitude of the configured
    type and stop here if this is the case.
 3. Iterate through all origins and magnitudes of the corresponding event.
 4. Copy the latest magnitude of the configured type to the origin referenced by
    the new origin reference just received.
 5. Send the magnitude copy to the messaging system.
 6. Let scevent decide whether the new magnitude becomes preferred.

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
            self.magType = self.configGetString("magType")
        except RuntimeError:
            pass

        return True

    def printUsage(self):
        # Module specific information
        print(__doc__)

        # Parameters inherited from base class
        super().printUsage()

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

    @staticmethod
    def formatMag(mag):
        return f"{mag.type()} {mag.magnitude().value()} ({mag.publicID()})"

    @staticmethod
    def ellipsisJoin(s1, s2, len_max):
        len_s1 = len(s1)
        len_remain = len_max - len_s1

        # first and second string fit into len_max
        if len_remain >= len(s2):
            return s1 + s2

        ellipsis = "..."
        len_ellipsis = len(ellipsis)

        # no space for ellipsis: truncate first string at the back
        if len_max <= len_ellipsis:
            return s1[:len_max]

        # first string exceeds max len: truncate first string at the front and
        # append ellipsis
        if len_remain <= len_ellipsis:
            return s1[: (len_max - len_ellipsis)] + ellipsis

        # trunctate second string at the front
        len_remain -= len_ellipsis
        return s1 + ellipsis + s2[-len_remain:]

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
        ci.setVersion(self.ellipsisJoin("clone of ", magnitude.publicID(), 64))
        magCopy.setCreationInfo(ci)

        # add magnitude to origin
        origin.add(magCopy)

        # send notifier
        msg = datamodel.Notifier.GetMessage()
        datamodel.Notifier.SetEnabled(wasEnabled)
        self.connection().send(msg)

        logging.info(
            f"Added magnitude {self.formatMag(magCopy)} as copy of "
            f"{magnitude.publicID()} to origin {origin.publicID()}"
        )

    def loadOriginWithMagnitudes(self, originID):
        origin = self.query().getObject(datamodel.Origin.TypeInfo(), originID)
        origin = datamodel.Origin.Cast(origin)
        if not origin:
            raise ValueError(f"Could not load origin {originID}")

        # load magnitudes of origin
        magCount = self.query().loadMagnitudes(origin)
        logging.debug(f"Loaded {magCount} magnitude(s) of {originID}")

        return origin

    def processOriginReference(self, eventID, originReference):
        # load origin for reference
        originID = originReference.originID()
        logging.info(f"Processing origin reference {originID} of event {eventID}")
        origin = self.loadOriginWithMagnitudes(originID)

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
        event = self.query().getObject(datamodel.Event.TypeInfo(), eventID)
        event = datamodel.Event.Cast(event)
        if not event:
            raise ValueError(f"Could not load event {eventID}")

        orefCount = self.query().loadOriginReferences(event)
        logging.debug(
            f"Searching magnitude of type {self.magType} in {orefCount} origins of "
            f"event {eventID}"
        )

        # Search for latest magnitude of given type in all referenced origins. Store
        # pointer to origin containing latest magnitude to prevent NPE in clone()
        # of copyMagnitude.
        latestMag = None
        latestMagOrigin = None
        for iOriginRef in range(event.originReferenceCount()):
            templateOriginID = event.originReference(iOriginRef).originID()
            if templateOriginID == originID:
                continue

            try:
                templateOrigin = self.loadOriginWithMagnitudes(templateOriginID)
            except ValueError as e:
                logging.warning(e)
                continue

            for iMag in range(templateOrigin.magnitudeCount()):
                mag = templateOrigin.magnitude(iMag)
                if mag.type() == self.magType and (
                    not latestMag or self.createdAfter(latestMag, mag)
                ):
                    if latestMag:
                        logging.debug(
                            f"Found newer template magnitude: {self.formatMag(mag)}"
                        )
                    else:
                        logging.debug(
                            f"Found first template magnitude: {self.formatMag(mag)}"
                        )
                    latestMag = mag
                    latestMagOrigin = templateOrigin

        if latestMag and latestMagOrigin:
            self.copyMagnitude(latestMag, origin)
        else:
            logging.info(
                f"Event {eventID} does not contain an origin with a magnitude of type "
                f"{self.magType}"
            )

    def addObject(self, parentID, scobject):  # pylint: disable=W0237
        # process origin references added to events
        originReference = datamodel.OriginReference.Cast(scobject)
        if not originReference:
            return

        try:
            self.processOriginReference(parentID, originReference)
        except Exception as e:
            logging.error(
                f"Error processing origin reference {originReference.originID()} "
                f"of event {parentID}: {e}"
            )


def main():
    app = MagnitudeCopy(len(sys.argv), sys.argv)
    return app()


if __name__ == "__main__":
    sys.exit(main())
