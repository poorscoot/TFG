Description
    This project provides conformance tests to assess to what degree does an implementation cover basic and optional
    characteristics of the Q4S protocol (https://datatracker.ietf.org/doc/draft-aranda-dispatch-q4s/).

Requirements
    You need to install g++, python, make and build-essentials.

General instructions
    1. Download the implementation you want to test and put the contets into the "Implementaciones" folder.
    2. Modify the socket values in both the implementation and the "Socket" folder in order to allow them to communicate.
    3. Head to the "TestCasesInter" and "TestCasesIntra" folders in a terminal and input "make".
    4. Compile the implementation.
    5. Head over to the main folder and use the python scripts to launch the test.
    (In order to use the TestCasesInter tests (1-15) you must first open the implementation server, in the TestCasesIntra (16-33) you must first open the test and then the implementation client)
    6.The test will show in the terminal Success or Failure, according to the results it obtained. In the case of a test that hasn't shown any info in a minute, the test should be considered a failure and be closed.(This will be fixed in the future)

    
