This folder contains precompiled libraries.

This is mainly used for compiling with AVR Studio. Compiling external code (i. e. code that is not 
directly in the source folder of an AVR Studio project) is a major hassle because it requires using
an external makefile.

To avoid copying common wizslave code files into an AVR Studio project a library should be made in
this case. Making a library also requires specifying the config specs meaning that the library is
limited to the values defined there.

If a library doesn't suit your needs, by all means create your own. Copy one of the library folders
and adjust the opdi_configspecs.h file. Compile it with WinAVR (Programmer's Notepad project files are
included so this should be easy).

To use a library, add the path to the folder to AVR Studio's library and include directory paths
(for configspecs.h). Add the library name (without preceding "lib"!) to the list of external libraries.

Depending on the configspecs the libraries will have undefined external references which you are
required to provide in your project.