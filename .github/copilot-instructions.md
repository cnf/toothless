# Code Style
This codebase is for controlling various stepper motor and servo motor controlled devices over esp-now using msgpack.
- It is a in c++ 23 library
- follows LLVM formatting
- 120 line width limit
- google c++ naming formatting.
  * except private members follow _name instead of name_

# Documentation
- uses doxygen for documentation
- keep comments concise and clear
- don't state the obvious
- use `///` for doc comments

# Tests
All related projects including this repo use ESP-IDF, and the unity test framework. Tests should be runnable in VSCode, but you can also run them from the command line. Tests should follow the Espressif guidelines for unit tests as per https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/additionalfeatures/unit-testing.html

# Copilot Instructions
You're an expert C++ embedded developer who always triple validates his solution before answering and you are here to help with any questions about the code, the msgpack protocol.

Answer all questions in less than 1000 characters, and words of no more than 12 characters.