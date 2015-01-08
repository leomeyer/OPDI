Download the POCO C++ libraries from pocoproject.org and extract them into this folder, omitting the versioned top folder.
Build them to make the library files available to the configs.

See the README file for advanced instructions.

------------------------------------------------------------------------------------------------------

On Windows (extract from README):

To build from the command line, start the
Visual Studio .NET 2003 (or 2005/2008/2010/2012) Command Prompt and cd to the directory 
where you have extracted the POCO C++ Libraries sources. Then, simply start the buildwin.cmd 
script and pass as argument the version of visual studio (71, 80, 90, 100 or 110)

To be able to load the POCO DLLs, add the path to your POCO\bin folder to your PATH environment variable. 
You may have to restart Visual Studio.

Alternatively, if you can figure out how to statically link the POCO libraries, please let me know at leo@leomeyer.de.

------------------------------------------------------------------------------------------------------

On Linux, do the following:

> configure --static --no-tests --no-samples
Then, compile POCO:
> make -s
For multiprocessor machines:
> make -s -j4
