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
