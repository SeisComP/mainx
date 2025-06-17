scmvx visualizes the current information of earthquakes or earthquakes loaded
database as well as current station information including:

* trigger status,
* ground motion,
* station quality,
* station configuration issues.

All stations and events are visualized in a map. The map can be customized by
global :confval:`scheme.map` parameters and additional layers can be added as
outlined in the :ref:`GUI documenation <global_gui>`. scmvx provides multiple
:ref:`tabs <sec-scmvx-tabs>`:

* :ref:`Network <sec-scmvx-network-tab>`: Maps showing events and network
  information including station configuration issues.
* :ref:`Ground motion <sec-scmvx-gm-tab>`: Map with events and stations. The
  color of stations symbols represents recent ground motion calculated by scmvx
  in a configurable time window.
* :ref:`Quality control <sec-scmvx-qc-tab>`: Map with events and stations. The
  color of stations symbols represents recent waveform quality control
  parameters calculated by :ref:`scqc`.
* :ref:`Events <sec-scmvx-events-tab>`: Event list updated in real time and
  allowing to filter events and to load historic events from database.


.. _sec-scmvx-tabs:

Tabs
====

Section is upcoming.


.. _sec-scmvx-network-tab:

Network
-------

Section is upcoming.


.. _sec-scmvx-gm-tab:

Ground motion
-------------

Section is upcoming.


.. _sec-scmvx-qc-tab:

Quality control
---------------

Section is upcoming.


.. _sec-scmvx-events-tab:

Events
------

Events are shown as they arrive in real time and may be interactively loaded
from database.
During startup events from within a period of time are loaded and shown
according the configuration of :confval:`readEventsNotOlderThan`.
More events are added in real time as they arrive.
Historic events can be loaded from database for time spans and other filter
criteria.
The loaded events can sorted interactively by clicking on the table header.
Events out of scope can be hidden based on region, event type of agency ID.
In contrast to other event lists, e.g. in :ref:`scolv` or :ref:`scesv`, this
event list only gives access to parameter of the preferred but no other origins
of events.

.. _fig-scmvx-events-tab:

.. figure:: media/scmvx-events-tab.png
   :width: 16cm
   :align: center

   Events tab


Hotkeys
-------

.. csv-table::
   :header: Hotkey, Description
   :widths: 30,70
   :delim: ;

   C; Center map around latest event upon event update
   G; Show map base layer in gray scale
   N; Reset view to default
   :kbd:`F1`              ;Open SeisComP documentation in default browser
   :kbd:`Shift + F1`      ;Open scmvx documentation in default browser
   :kbd:`F2`              ;Open dialog for connecting to database and messaging
   :kbd:`F6`              ;Toggle latest event information on map
   :kbd:`F7`              ;Toggle legend
   :kbd:`F8`              ;Toggle station issues in Network tab
   :kbd:`F9`              ;Toggle station annotation
   :kbd:`F10`             ;Switch to event list (Events tab)
   :kbd:`F11`             ;Toggle full screen mode
   :kbd:`CTRL + F`        ;Search station
   :kbd:`CTRL + O`        ;Open event parameter XML file
   :kbd:`CTRL + Q`        ;Quit scmvx
   :kbd:`Shift + Arrows`  ;Move focus of map
   :kbd:`+`               ;Zoom in in map
   :kbd:`-`               ;Zoom out in map
   Mouse wheel            ;Zoom in or out in map
   Mouse double click     ;Center map
   Right mouse button     ;Open context menu


Use Cases
=========


Get station information, detail issues
--------------------------------------

#. Navigate to the Network tab
#. Position the mouse above a triangle representing a station. The selected
   station is highlighted. Zoom in if events are overlapping.
#. Click your left mouse button for opening the station info widget.
   Data and potential configuration issues are shown.


Search for and show an event
----------------------------

#. Navigate to the Events tab load events from database in a relevant time range.
   You may narrow down the database search through the filter button. After
   events are loaded the list may be limited by hiding irrelevant events and you
   may change the sorting of the event table by clicking on the header.
#. Identify the event and double-click on the event line to load the parameters.
   You will immediately switch to a map centered around the selected event.


Get event information
---------------------

#. Position the mouse above a circle representing the location of an event.
   Zoom in if events are overlapping.
#. Click the left mouse button for opening the event object inspector.


Set preliminary origin
----------------------

*Upcoming feature, not yet supported*

#. Position the mouse in the map
#. Press the middle mouse button
#. Set date & time and latitude, longitude & depth
#. Press "Create" to open the origin in another GUI, e.g., scolv which must
   be running already.


Search station/network
----------------------

#. Press :kbd:`CTRL + F` to open the search window.
#. Type any string from a station and/or network name in the input field or just
   select a station from the list.
#. Double click in a station in the list to center the map at this location.


Command-Line Examples
=====================

* Real-time view of events and stations on a local server

  .. code-block:: properties

     scmvx -H localhost -I slink://localhost --debug

* Offline view of event parameters given in an XML file. Inventory is read from
  database.

  .. code-block:: properties

     scmvx -d localhost -i events.xml --debug
