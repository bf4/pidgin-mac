This is the package containing the build environment for Pidgin.

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


Install the Gtk+ framework (together with Cairo and GLib, if not included in the framework already) to /Library/Frameworks/. At the time of writing (2009-03-10), the framework included in this archive should be the newest one (build from SVN). If that one is outdated, you can download it from www.gtk-osx.org.

Now, you should be able to load the Xcode project in pidgin-macosx and build Pidgin.
