# Cinema 4D Connector - Documentation

This document contains documentation for the Cinema 4D Connector project.

## Installation

In order to be able to use all the features it is necessary to install the following two extensions:

- The Cinema 4D plugin, downloadable [here](https://github.com/PluginCafe/Cinema4D_Connector-Cinema4D_Plugin/releases). Once downloaded, extract the archive to the Cinema 4D S26+ plugins folder. You then need to activate the extension in the Cinema 4D preferences in the `Extensions | Code Exchange` menu, activate the WebSocket Json checkbox.

- The `Cinema 4D Connector` extension for Visual Studio Code, directly accessible in the Visual Studio code marketplace, or download it [here](https://github.com/PluginCafe/Cinema4D_Connector-VisualStudioCode_Extension/releases).

## Settings

In Cinema 4D preference within the `Extensions | Code Exchange` menu.

**WebSocket Json**: If enabled, the WebSocket server is started. The server will then start automatically during Cinema 4D startup. Once the server started, you can connect an IDE instance to it and shares codes.

**Port**: The port the current WebSocket server is listening to.

In Visual Studio Code preference within the `Extensions | Cinema 4D` category.

**c4d.path**: Path to the Cinema 4D directory used for autocompletion and debugging. If not defined or invalid, it is automatically defined when Visual Studio Code and Cinema 4D first connect.

**c4d.ip**: IP address used to connect to Cinema 4D.

**c4d.port**: Port used to connect to Cinema 4D. This should be the same than the one in Cinema 4D preference.

**c4d.template**: Path to the directory containing the python scripts used as template.

## Feature Description

**Connection states with Cinema 4D**: In the lower left corner of the Visual Studio Code windows, a status bar indicates the current connection status with Cinema 4D. Runing the `Toggle Connection with Cinema 4D` command or clicking on the items in the status bar toggles the current connection status. If the status bar shows "C4D ✓", it means that the connection is active and that the communication is going well. Otherwise, "C4D X" means that the connection is not active. 

**Send script from Cinema 4D to Visual Studio Code**: The command in the "File" menu of the Cinema 4D Script Manager allows you to send the contents of the active script in the Script Manager to all IDEs connected to Cinema 4D. In order to use this feature Cinema 4D and Visual Studio Code must be connected.

**Load Script in Script Manager**: Load the active script from the Visual Studio Code editor into the Cinema 4D Script Manager. In order to use this feature Cinema 4D and Visual Studio Code must be connected. Lastly, depending on where the script is saved in Visual Studio Code, different behaviour is expected if the file is:

- Saved from file to disk: Works as expected, saving the file refreshes the script content in both the IDE and Cinema 4D.
- A temporary script from Visual Studio Code: A new temporary file will be created in Cinema 4D. This file will be added to Visual Studio Code and to communicate with Cinema 4D you will need to use this file instead of the previous temporary file.
- A temporary script from Cinema 4D: Works as expected, CTRL+S will send the content to Cinema 4D. Autocomplete does not work.

**Execute in Cinema 4D as a Script in Script Manager**: Run a script directly in Cinema 4D. In order to use this feature, Cinema 4D and Visual Studio Code must be connected. Lastly, depending on where the script is saved in Visual Studio Code, a different behaviour is expected if the file is:

- Saved to disk or from a temporary Cinema 4D script: Update the contents of the script in Cinema 4D and run it.
- A temporary script from Visual Studio Code: Create a new temporary python script in Cinema 4D. Run it and delete it from Cinema 4D.

**Debug in Cinema 4D as a Script in Script Manager**: Start a debugging session for the given script to Cinema 4D. Encrypted Python files (.pypv, .pype) are ignored and can't be debugged. In order to use this feature, Cinema 4D and Visual Studio Code must be connected. Works only for file saved on the disk.

**Load Cinema 4D Script Template**: Loads a template script, corresponding to a python file from the folder defined by the `c4d.template` option.

**Python Console output**: Once Visual Studio Code is connected to Cinema 4D, if new content appears in the Python console, it will also be displayed in a Visual Studio Code console called "Cinema 4D".

**Syntax highliting for \*.res and \*.str files**: La syntaxe pour les fichiers avec lextension .str et .res dispose d'une colorisation syntaxique. Pour les fichier .str, en cas de syntaxe invalide, celle-ci est coloré en rouge.


## Known Issues

- Autocompletion does not work for the `maxon` package.
- Autocompletion does not work for temporary scripts from Cinema 4D, those whose path begins with `Root@`, e.g. `Root@12345678/Scripts@12345678/untilted.py.`
- Autocompletion for methods from the `c4d` package will generate incomplete default argument if this argument is part of the `c4d` package, e.g. the automcpletion will output only `BaseContainer` while it should be `c4d.BaseContainer`.
- When the `Load Script in Script Manager` command is used on an untitled file, it creates a new temporary file in Cinema 4D and this is returned to Visual Studio Code. This file should be used to exchange data to/from Cinema 4D.
- The first debugging session will show a message about the deprecated use of `ptvsd`, this is a false positive and can be ignored.
