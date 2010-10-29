This is the package containing the build environment for Pidgin.

I would say it's already pretty usable (and I am using it myself). The MacOSX integration could be a bit better but I'll just wait there for the Gtk+ MacOSX port, they are implementing on related stuff (IGE Mac).

Directory structure:

* pidgin-src 		: -> link to current sources
* pidgin-src-orig	: -> link to current original sources
* pidgin-2.5.5		: the patched Pidgin sources
* pidgin-2.5.5-orig	: the original sources (needed there for diffing)
* pidgin-macosx		: Xcode project files
* pidgin-otr-3.1.0	: OTR plugin
* gnutls-2.6.4		: patched GnuTLS sources
* gfire-0.7.1		: patched Gfire sources
* pidgin-facebookchat-1.47 : patched Facebookchat sources


Install the Gtk+ framework (together with Cairo and GLib, if not included in the framework already) to /Library/Frameworks/. At the time of writing (2009-03-10), the frameworks included here should be the newest one (build from SVN). If that one is outdated, you can download it from www.gtk-osx.org.

Now, you should be able to load the Xcode project in pidgin-macosx and build Pidgin.

This Pidgin port for MacOSX is maintained by Albert Zeyer.
Contact me at albert.zeyer@rwth-aachen.de.
Or visit my homepage: www.az2000.de

This project was mirrored/forked by Benjamin Fleischer to help create a more modern build script for native mac Pidgin with voice and video.

We'll likely need to add avahi and gstreamer and farsight2 (and it's plugins).  

Looks helpful:
http://www.salixos.org/forum/viewtopic.php?f=20&t=837
http://www.pidgin.im/download/mac/
http://developer.pidgin.im/wiki/Installing%20Pidgin#CanIrunPidginonMacOSX
http://developer.pidgin.im/wiki/vv
http://www.linuxquestions.org/questions/slackware-14/pidgin-voice-and-video-749576/
gstreamer-0.10.25 ,
gst-plugins-base-0.10.25,
gst-plugins-good-0.10.17,
gst-plugins-bad-0.10.17
gst-plugins-ugly-0.10.13,
gst-ffmpeg-0.10.9,
farsight2-0.0.16
http://www.amsn-project.net/forums/index.php?topic=6395.0
http://www.amsn-project.net/wiki/Compiling_aMSN
http://trac.adium.im/wiki/PidginVV
Getting Farsight to work on the Mac:  http://amsn-project.net/wiki/Farsight.
Files needed:  http://amsn.svn.sourceforge.net/viewvc/amsn/trunk/amsn/utils/macosx/gstreamer/
Pidgin Wiki entry on "Voice and Video":  http://developer.pidgin.im/wiki/vv
Pidgin Wiki entry on the "vv libPurple API":  http://developer.pidgin.im/wiki/vvAPI
Information on the 2008 Pidgin GSoC "Voice and Video Support" project:  http://developer.pidgin.im/wiki/GSoC2008/VoiceAndVideo
Pidgin-Devl list thread on the 2008 GSoC "Voice and Video Support" project:  http://pidgin.im/pipermail/devel/2008-May/005889.html
