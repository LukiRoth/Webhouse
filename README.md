# Raspberry-Pi Webhouse Project

## Introduction

This project is a comprehensive solution for a Raspberry-Pi based smart home system, known as Webhouse. It encompasses web-based interfaces, server backend logic, and documentation, all aimed at providing control and monitoring capabilities for various home utilities.

## Project Structure

The project is organized into several directories, each serving a different role in the application:

- **01_Webpage**: Contains the web interface files for the Webhouse system, allowing users to interact with the home utilities through a web browser.

- **02_Server**: Holds the server-side application that interfaces with the Raspberry-Pi hardware to control the Webhouse utilities.

- **03_Documentation**: Includes detailed documentation of the project, outlining its features, architecture, and user guides.

- **04_Template**: Contains template files and configuration settings used across the project.

Each directory has its own `README.md` providing detailed explanations and specific instructions related to the contents.

## Compilation and Installation

To compile and install the Webhouse system:

1. Navigate to the `02_Server` directory.
2. Run the `Makefile` which will compile all necessary source files and generate the executable.

   ```bash
   make
3. Start the server by executing the generated binary:

   ```bash
   sudo ./Template
4. Navigate to the `01_Webpage` directory. Make sure that the IP address of the Raspberry-Pi is correctly configured in the `app.js` file.
5. Open the webhouse.html file from the 01_Webpage directory in a web browser to access the Webhouse interface.

## Usage
Upon launching the server, you can control the various utilities like lights, heater, and TV through the web interface. Real-time updates and control commands are communicated using WebSockets, ensuring a responsive user experience. Before using the system, ensure that the Raspberry-Pi is connected to the Internet and the server is running. 
Also make sure that the IP address of the Raspberry-Pi is correctly configured in the `app.js` file in the 01_Webpage directory.

For more detailed usage instructions, refer to the README.md files within each directory and the 05_Documentation folder.

## License
This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgements
Thanks to Elham Firouzi for providing the initial template.