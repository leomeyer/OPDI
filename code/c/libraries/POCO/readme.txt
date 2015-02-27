Download the POCO C++ libraries from pocoproject.org and extract them into this folder, omitting the versioned top folder.
Build them to make the library files available to the configs.

See the README file for advanced instructions.

------------------------------------------------------------------------------------------------------

On Windows (extract from README):

To build from the command line, start the
Visual Studio .NET 2003 (or 2005/2008/2010/2012) Command Prompt and cd to the directory 
where you have extracted the POCO C++ Libraries sources. Then, simply start the buildwin.cmd 
script and pass as argument the version of visual studio (71, 80, 90, 100 or 110).

The Windows configs should link the POCO libraries statically. This avoids the need to deploy the DLLs as well.
If you want to use the DLLs remove the POCO_STATIC compiler directive from the project settings and copy the DLLs to the resulting exe's folder.

------------------------------------------------------------------------------------------------------

On Linux, do the following:

> configure --no-tests --no-samples
Then, compile POCO:
> make -s
For multiprocessor machines:
> make -s -j4
Install the libraries
> sudo make -s install
Reload LD cache
> sudo ldconfig

The linux configs should link the POCO libraries dynamically. This avoids problems ("Using 'gethostbyname' in statically linked applications requires at runtime the shared libraries from the glibc version used for linking").
