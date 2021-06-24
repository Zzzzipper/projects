### remoteq ###

1. Client-server application for receiving data and controlling the voltage meter simulator.
2. The server part is made using the c ++ language and standard libraries.
3. The client side - using the Qt framework version 5.12.3.
4. A socket of the AF_UNIX type is used for connection;
5. C ++ 17 standard.
6. The exchange protocol is implemented according to option 2 from [tasks](https://github.com/Zzzzipper/remoteq/blob/master/docs/%D0%97%D0%B0%D0%B4%D0%B0%D0%BD%D0%B8%D0%B5%20%D0%BF%D1%80%D0%BE%D0%B3%D1%80%D0%B0%D0%BC%D0%BC%D0%B8%D1%81%D1%82%D1%83_23_04_20.pdf) to perform test work
7. The assembly and testing was carried out in the Kubuntu 20.04 LTS environment

##### Development software requirements #####

1. Install cmake from distribution packages. For Ununtu:
   ``
   sudo apt-get install cmake
   ``
2. If installed with a version older than Qt5.12.3, it is better to update the installation to the required version;
3. If several distributions are used, then it is better to select the one you need using `` qtchooser '' and the environment variable `` QT_SELECT '' to work with `` qmake '' from the command line.

##### Build #####

1. After installing the necessary software and cloning the project, you need to go to the root directory of the project and run the script on the command line
``
./build.sh
``
2. After that, if no errors occur, the executable files `` remoteq '' (this is the server) and `` client '' should be found in the root directory `` bin ''.
3. Both projects are configured to work with them in `` qtcreator '' - you can build and debug. For the development environment to work successfully with the project for `` cmake '', you need to correctly configure the project settings in terms of the sequence of executing the `` make '' commands, deleting and moving to the required directory. For example, the file `` CMakeLists.txt.user '' is uploaded to the repository - you can copy the desired section from it;

##### Refinement #####
1. To add a new command, you must first add its index to the enumeration `` uds_command_type '' in the file `` include / common.h '' at the root of the project;
2. Then, add a handler (lambda function) in the src / headmeters.cpp file in the operate method - according to the template of those that are already there;
3. In the client, add `` Q_INVOKABLE '' methods for transferring data to the Qml engine;
