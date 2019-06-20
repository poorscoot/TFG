Description

    This project provides conformance tests to assess to
    what degree does an implementation cover basic and
    optional characteristics of the Q4S protocol
    (https://datatracker.ietf.org/doc/draft-aranda-dispatch-q4s/).

    Part of the code used here belongs to the Q4S
    implementation by the HPCN-UAM
    (https://github.com/hpcn-uam/q4s).

Requirements

    You need to install g++, python, make and build-essentials.

General instructions

    1. Download the implementation you want to test and put
    the contets into the "Implementaciones" folder. (Create
    this folder in same one as the scripts for easy access)

    2. Modify the socket values in both the implementation
    and the "Socket" folder in order to allow them to
    communicate.

    3. Head to the "TestCasesInter" and "TestCasesIntra" 
    folders in a terminal and input "make".

    4. Compile the implementation.

    5. Head over to the main folder and use the python
    scripts to launch the test.
    (In order to use the TestCasesInter tests (1-15) you
    must first open the implementation server, in the 
    TestCasesIntra (16-33) you must first open the test and
    then the implementation client)
    
    6.The test will show in the terminal Success or Failure,
    according to the results it obtained. In the case of a
    test that hasn't shown any info in a minute, the test
    should be considered a failure and be closed.(This will
    be fixed in the future)

Tests (Only the tests with a python script are functional)

    1.Tests wether or not the server answers a BEGIN message.

    2. Tests wether or not it answers with the QoS 
    characteristics proposed. (Optional).

    16. Tests wether or not the client sends a BEGIN message.

    17. Checks if the client sends the values proposed
    when sending a BEGIN. (Modify the test in order for the
    ones in there to match the ones you ask for)

    18. Checks if the client resends a BEGIN message if the
    first is not answered. (Optional)

    19. Tests wether or not the client starts Stage 0
    by sendind a READY 0 after the server accepting its
    BEGIN.

    20. Checks if the client resends a READY message if the
    first is not answered. (Optional)

    23. Checks if the client starts Stage 1 without first
    going through Stage 0 if it isn't necessary (Latency
    and jitter values at 0).

Besides checking if the implementations cover these properties,
it also test if it understands the message 
format presented at the implementation and if it messages
also follow this format.