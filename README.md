# kleine (MuSE Tests)

A cross-platform, audio-enabled C++ console code project, using CMake and Klang, extended with a series of automateed C++ tests, designed to tests students' knowledge of the C++ language and object-oriented programming.

This project is used on the Programming in C++ module, part of pointblank's [BSc(Hons) Music Production and Software Engineering](https://www.pointblankmusicschool.com/course-finder/all/all/music-software-engineering/all/) degree.

For questions or issues, please email: chris.nash@pointblankmusicschool.com.

## Task

Complete a series of code tests based on specific concepts in the C++ language.

## Purpose

This assessment probes your knowledge of the C++ language and programming techniques through a series of automated tests. 

Automated tests are widely used in industry to ensure code works as expected. For example, a test may check a piece of code outputs the expected response for a given input. In this assessment, you will supply a series of code fragments that are run through specific tests that check specific criteria are met and award marks appropriately.

Each test is designed around a specific concept in C++ (files, input, output, objects), awarding multiple marks for demonstrating both basic and advanced concepts or techniques. You can complete the tests as often as you like, at any point during the module, and receive the mark instantly, but must submit your final solutions are the end of the module, before the deadline.

You do not have to complete all tests to pass the assessment.

## Requirements

- Write C++ code to pass automated tests.
- There are 12 tests across three themes (Basic, STL/Files, OOP), offering a total of 50 marks.
- You need to obtain 20 marks (40%) to pass, across all the tests. You do not need to complete every test to pass, nor every part of a test.
- Higher marks are obtained by successfully completing more tests and meeting more test criteria. It is possible to achieve 100% (50 marks) - though some marks are intentionally challenging.
- The complete mark / test criteria is available [here](tests/tests.md).
- Submit your code in a single file ([tests.cpp](tests/tests.cpp)) using the VLE before the deadline.

## Guidance

- These automated tests cover all the skills you need for the project. You may find completing the tests helps prepare you for the project.
- To gain access to the tests, checkout / clone the tests branch of the nashaudio/kleine  repository on GitHub. This version of the project contains a subfolder called /tests that provides all the templates, tests, and objects that you need for the assessment. The code project is based on CMake, a technology that allows you to work with your preferred IDE by generating files for different platforms and compilers dynamically:
  - **Visual Studio 2022/2026** has built-in CMake support, which enables you to directly open the folder, edit code, run, and debug. Alternatively, a Visual Studio project file (.vsproj) is provided in the /projects folder.
  - **Visual Studio (VS) Code** supports CMake projects through the CMake Extension (open the folder, click CMakeLists.txt, and VS Code will offer to install the extension automatically). If code doesn't run automatically when pressing F5 or Running the Debugger, use the CMake tab on the left and select the Debug / Launch option.
  - **Xcode** does not support CMake natively. An Xcode project file (.xcodeproj) is provided in the [/build](build) folder.
- Details of each test, criteria, and the marks available are provided in the [tests.md](tests/tests.md) document, which can be easily viewed here on GitHub.
- Your solutions should be written in the [tests.cpp](tests/tests.cpp) file, in the provided code template project. For some tests, named functions are provided for you to complete - do not change the names, arguments, or return types of these functions. Other tests may require you to declare/define objects with specific names.
- When developing function-based tests (all tests except 9, 10, and 12), simply call the function to run the code from the main() function in [kleine.cpp](kleine.cpp):

      hello( );    // run the "Hello, World!" function
      beats() ;    // run the "Beats by Dr. Nash" function 

- To run a test on a piece of code, use the test:: namespace:
  
      test::hello( );    // run the "Hello, World!" test 
      test::beats() ;    // run the "Beats by Dr. Nash" test 
      test::object() ;   // run the "Objection Noted" test

- Running a test will provide the mark and break down which criteria your code code passed or failed:

      [PASS] Contains the word, "hello",
      [PASS] Contains appropriate capitalisation ("e.g. Hello").
      [FAIL] Contains appropriate punctuation (commas, exclamation mark)
      [FAIL] Supports a user-specified name.

      Test marks: 2/4

- To run all tests and receive a total mark, you call:
  
       test::all( );    // run all 12 tests in sequence

- Some tests provide objects/files for you to work with, others require you to create your own objects.
- Try to avoid platform-specific (e.g. Windows / MSVC only) code. The recommended platform to attempt the tests is Visual Studio 2022 on Windows, which will be used by the markers, but the code should be compatible with other platforms and compilers.
