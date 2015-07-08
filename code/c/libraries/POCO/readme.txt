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

> ./configure --no-tests --no-samples
Then, compile POCO:
> make -s
For multiprocessor machines:
> make -s -j4
Install the libraries:
> sudo make -s install
Reload LD cache:
> sudo ldconfig

The linux configs should link the POCO libraries dynamically. This avoids problems ("Using 'gethostbyname' in statically linked applications requires at runtime the shared libraries from the glibc version used for linking").

------------------------------------------------------------------------------------------------------

On Raspberry Pi, make sure that the libssl-dev library is installed:
> sudo apt-get install libssl-dev

Compile without ODBC and MySQL support:
> ./configure --no-tests --no-samples --omit=Data/ODBC,Data/MySQL
Then, compile POCO:
> make -s
Compiling can take quite a long time. Check for errors; install missing libraries if necessary.
Install the libraries:
> sudo make -s install
Reload LD cache:
> sudo ldconfig

If you want to cross-compile for the Raspberry Pi, follow these steps:

1. Install a cross-compiler toolchain on your compile system and make sure that it works. For example: https://github.com/raspberrypi/tools
Note: Currently it is not recommended to use the popular crosstools-ng for this because of problems downloading library dependencies (eglibc.org SVN server is unreliable).

2. Create a build configuration for POCO:
> cd build/config
> cp ARM-Linux RaspberryPi
Edit the file RaspberryPi:
In general settings, remove the lines starting with "STLPORT" and "OPENSSL"
In general settings, change the tool (Note: this assumes that you are using the RaspberryPi cross compiler from Github mentioned above):
TOOL = arm-linux-gnueabihf 
In System Specific Flags, remove the following flags:
-I$(STLPORT_INCLUDE) -I$(OPENSSL_INCLUDE)
In System Specific Libraries, remove the following libraries:
-L$(STLPORT_LIB) -L$(OPENSSL_LIB) -lstlport_arm-linux-gcc
Save and return to POCO root:
> cd ../..

3. Configure POCO to use the new build configuration:
> ./configure --config=RaspberryPi --no-samples --no-tests --omit=CppUnit,CppUnit/WinTestRunner,Crypto,NetSSL_OpenSSL,Data,Data/SQLite,Data/ODBCData/MySQL,MongoDB,PageCompiler,PageCompiler/File2Page

4. Build:
> make -s
