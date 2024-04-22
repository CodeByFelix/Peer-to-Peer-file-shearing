# Peer-to-Peer-file-shearing
This is a console app that supports shearing of files between devices connected on same network. The software has create connection and join connection. 
When one user creates connection, another user joins the connection by providing the ip address of the user that created the connection.
After a connection has being established between the two devices, the user that created connection will innitiate the file shearing by entering the name of the file to send to the other user. When the file is found, it first sends the file name and file size as header befor sending the files in packets of 100Kb so that the receiver can save the file with its exact name and keep track of the size received and the size remainig in percentage.
When the sending is completed, the receiver saves the file so that it can be accessed at any time.

The basic resources i made use of in this software is the file stream and winsock2 library.

The app runs only on windows
