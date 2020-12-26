Cam-View is an open source system for capturing video from one or more network-attached video cameras
and for displaying, storing locally and archiving remotely captured images.
Cam-View can run on inexpensive PC hardware and draw images from inexpensive cameras. It was devel-
oped on Ubuntu but should be easily portable to any Linux distribution and with minor effort any Unix-based
system.

Cam-View is suitable for capturing, storing and viewing security video footage in a small setting such as
a house or apartment up to a medium to large setting such as a multi-floor office building. A modestly
equipped PC (sub $1000, no GPU) should be able to capture and process image data from at least 20 or
30 cameras concurrently.

Key features of Cam-View include:

1. Inspecting each captured image to determine whether it should be stored or discarded.
2. Only storing images if either motion is detected or a threshold time interval has elapsed since the last
stored image.
3. Displaying stored images via a web UI – all user interaction, once the system is installed, is via a web
browser.
4. Migrating image files across three storage tiers:
  - A local, high performance, non-persistent ramdisk. This is where images are initially captured.
  - A local persistent disk (solid state SSD or spinning HDD). This is where interactive viewing or
video downloads come from.
  - A remote archive. This ensures image retention even if the local storage server is lost or dam-
aged.
5. Dealing gracefully with inexpensive, unreliable and poorly connected cameras, for example by period-
ically attempting to reconnect.
6. Support for multi-tenancy, in both the physical building sense and logical access sense of the word.
One process can capture video data from multiple cameras in a building. Different URLs are used to
present video to different tenants – each from a distinct subset of cameras.
7. Interactive and off-line viewing. Interactive via a web UI, which allows a viewer to navigate through
captured images (i.e., those where motion was detected or time had elapsed) or to download a video
file or image archive consisting of the same files.
